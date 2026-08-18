// Render-view injection for Portal 2 (Source Engine, client.dll).
//
// Hook target: CViewRender::RenderView(CViewSetup* view, CViewSetup* hudView,
// int clearFlags, int whatToDraw)  [__thiscall].  Discovered via MSVC RTTI:
// the CViewRender vftable, slot identified by the scissor-rect use of the
// CViewSetup int fields (x/y/width/height) - see .lab/NOTES.md.
//
// RenderView runs in the render phase, after the game has already produced the
// frame's CUserCmd / view angles (which drive aim, portal placement, traces).
// We mutate the CViewSetup the renderer is about to consume - its angles and
// origin only - so the player sees the head-tracked view while the game's own
// camera (cl.viewangles) is untouched. Look and aim stay decoupled for free.
//
// Engagement is gated on a PE-fingerprint build-profile registry (append-only;
// see the "Maintain compatibility across new patches" doctrine and
// builds/build_registry.h). On any client.dll the registry does not recognise,
// the hook is never installed and the game runs vanilla.

#include "camera_hook.h"

#include <Windows.h>
#include <cmath>
#include <cstdint>

#include "aim_state.h"
#include "angles.h"
#include "builds/build_registry.h"
#include "cameraunlock/hooks/hook_manager.h"
#include "cameraunlock/memory/pe_fingerprint.h"
#include "debug_log.h"
#include "plugin.h"
#include "source_math.h"
#include "view_setup.h"

namespace headtracking {

namespace {

// ----- Resolved-at-load state ----------------------------------------------
const builds::BuildProfile* g_profile = nullptr;

using RenderViewFn = void(__fastcall*)(void* ecx, void* edx, void* view, void* hud,
                                       int clearFlags, int whatToDraw);
RenderViewFn g_originalRenderView = nullptr;

// What the pose pipeline contributed to this frame's view, carried to the
// diagnostic line so it can report the delta alongside the resulting camera.
struct TrackingDelta {
    bool applied = false;
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;  // degrees, Source sign
    float x = 0.0f, y = 0.0f, z = 0.0f;           // Source units, camera basis
};

// Split-screen slot for this view. CViewRender::Render walks the valid
// split-screen players and calls RenderView once per player per frame, and the
// only difference visible from the CViewSetup is the viewport rect: the second
// player's tile is offset (horizontal split -> y > 0, vertical -> x > 0).
// Single-player and every render-to-texture view start at the origin, so they
// stay on player 1's feed.
//
// Fails toward player 1, deliberately. Routing an unexpected offset viewport to
// a slot that has no tracker configured would leave that view untracked with
// nothing in the log to explain it - the mod would look installed and dead. The
// rect offsets are inferred rather than dumped on the 2025-01-17 profile, and
// only the vertical split has been seen in the wild, so an offset rect we did
// not predict is a real possibility. When there is no second player, every view
// is player 1's; the offset is logged once so a surprise shows up in a report.
int SplitScreenSlot(const ViewSetup& view, int activePlayers) {
    const int x = view.RectX();
    const int y = view.RectY();
    if (x <= 0 && y <= 0) return 0;

    static bool s_loggedOffsetRect = false;
    if (!s_loggedOffsetRect) {
        s_loggedOffsetRect = true;
        HT_LOG("[view] first offset viewport rect=(%d,%d) -> %s", x, y,
               activePlayers > 1 ? "player 2's tracker" : "player 1 (no second player configured)");
    }
    return activePlayers > 1 ? 1 : 0;
}

// ----- Field of view --------------------------------------------------------
//
// CViewSetup::fov is the horizontal FOV, in degrees, the frame is actually
// rendered with. CViewRender::SetUpView has already taken the player's
// `fov_desired` - which Source defines against a 4:3 screen - and widened it
// for the real viewport by the time RenderView sees the struct. Confirmed in
// game: fov_desired 90 in a 1280x800 viewport arrives here as 100.39, which is
// 90 widened by (1280/800)/(4/3).
//
// So this field is both the value to read when something needs the camera's
// FOV, and the right place to change it. Everything the frame is built from
// comes out of this struct - including the world-to-screen matrix any screen
// projection of a world point has to go through - so an override written here
// needs nothing else kept in sync with it.
//
// The game's own knob only spans 75 to 90 (fov_desired's ConVar bounds; 120
// silently clamps to 90) and is FCVAR_USERINFO, so it is also sent to the
// server. Neither is true of a write into the render view. The INI value is
// expressed in the same 4:3-referenced units as fov_desired so a number from
// the console or an FOV guide transfers directly, and it is widened here with
// Source's own formula rather than written in raw.

// At and past this the projection degenerates - tan(fov/2) runs away - so an
// override that widens into it is refused rather than handed to the renderer.
constexpr float kMaxRenderableFov = 179.0f;

// Writes the configured FOVs into the render view. The world FOV and the
// viewmodel FOV are separate fields the engine widens the same way, so both
// take a 4:3-referenced INI value and go through the same conversion.
//
// A viewport that does not read as a rect disables the override for the rest
// of the session rather than being retried every frame: it means the profile's
// CViewSetup offsets do not fit this client.dll, and a projection built from
// the NaN that follows is a black screen, not a cosmetic fault.
void ApplyFovOverride(const ViewSetup& view, int slot, float worldFov, float viewmodelFov) {
    static bool s_disabled = false;
    if (s_disabled || (worldFov <= 0.0f && viewmodelFov <= 0.0f)) return;
    if (slot < 0 || slot >= Plugin::kMaxPlayers) return;

    const int w = view.RectWidth();
    const int h = view.RectHeight();
    if (w <= 0 || h <= 0) {
        s_disabled = true;
        HT_LOG("[view] FOV override disabled: viewport reads as %dx%d - the build profile's "
               "CViewSetup offsets do not fit this client.dll", w, h);
        return;
    }
    const float ratio = (static_cast<float>(w) / static_cast<float>(h))
                        * source::kReferenceAspectInverse;

    // `logged` is the caller's static, so each field reports once and again
    // whenever the widened value changes - a resolution switch, or a viewport
    // that turns out not to be the one the first frame showed. Kept per player:
    // split-screen tiles have different aspect ratios, so one shared value would
    // see the two viewports' FOVs alternate and log on every single frame.
    auto override_field = [&](float& target, float desired, const char* what, float& logged) {
        if (s_disabled || desired <= 0.0f) return;
        const float rendered = source::ScaleFovByWidthRatio(desired, ratio);
        if (!std::isfinite(rendered) || rendered <= 0.0f || rendered >= kMaxRenderableFov) {
            s_disabled = true;
            HT_LOG("[view] FOV override disabled: %s FOV %.2f widens to %.2f degrees at %dx%d, "
                   "which has no projection", what, desired, rendered, w, h);
            return;
        }
        if (logged != rendered) {
            logged = rendered;
            HT_LOG("[view] p%d %s FOV override: %.1f (as fov_desired) -> %.2f at %dx%d, "
                   "game was rendering %.2f", slot + 1, what, desired, rendered, w, h, target);
        }
        target = rendered;
    };

    static float s_loggedWorld[Plugin::kMaxPlayers] = {};
    static float s_loggedViewmodel[Plugin::kMaxPlayers] = {};
    override_field(view.Fov(), worldFov, "world", s_loggedWorld[slot]);
    override_field(view.FovViewmodel(), viewmodelFov, "viewmodel", s_loggedViewmodel[slot]);
}

// ----- Tracker -> Source axis mapping ---------------------------------------
//
// Every sign correction between the tracker frame and Source lives here, at
// the engine boundary. It must NOT be expressed as an INI `Invert*` default:
// the processor applies inversion BEFORE the asymmetric Z clamp, so an
// `InvertZ` used to flip the engine convention silently moves the generous
// LimitZ (0.40m) allowance onto the backward lean and leaves LimitZBack
// (0.10m) for leaning in. The direction still looks right, which is why that
// shape survives testing - the only symptom is that leaning in barely moves.
// The `Invert*` keys stay pure user preferences, defaulting to off.
//
// Calibrated in-game (see .lab/NOTES.md). Tracker frame, as the pipeline
// delivers it:
//     yaw   > 0  = head turns right   Source yaw   > 0 = turn left   -> negate
//     pitch > 0  = head looks up      Source pitch > 0 = look down   -> negate
//     roll  > 0  = head tilts left    Source roll  > 0 = tilt right  -> negate
//     x     > 0  = head moves left    Source right vector            -> negate
//     y     > 0  = head moves up      Source up vector               -> as-is
//     z     < 0  = head leans forward Source forward vector          -> negate
constexpr float kYawSign   = -1.0f;
constexpr float kPitchSign = -1.0f;
constexpr float kRollSign  = -1.0f;
constexpr float kPosXSign  = -1.0f;
constexpr float kPosYSign  =  1.0f;
constexpr float kPosZSign  = -1.0f;

// ----- Diagnostics ----------------------------------------------------------
//
// Dense at first (the first frames, then ~every 200), because that is where
// install-time faults show. Then it thins out to one line per ~2000 frames and
// stays there for the session: the failures worth catching are the late ones
// ("it drifted after an hour", "it stopped when I loaded a save"), and a burst
// that goes silent cannot see them. Thinning keeps a long session's log
// readable and keeps the render thread out of the log mutex.
constexpr int kBurstLines           = 6;     // opening frames logged unconditionally
constexpr int kEarlyLines           = 30;    // lines still treated as install-time
constexpr int kEarlyIntervalFrames  = 200;
constexpr int kSteadyIntervalFrames = 2000;

// Confirms the hook fires, the offsets resolve to a sane camera, and the
// head-tracking delta is being applied. Reads the CViewSetup after the delta
// has been written, so the line reports the view the frame will render with.
void DiagnosticLog(int slot, const ViewSetup& view, const TrackingDelta& delta) {
    // Per-slot counters: split-screen calls this once per player per frame, and
    // a shared counter lets one viewport win every throttle slot - the log then
    // silently covers one player and a bug report arrives half blind.
    static int s_count[Plugin::kMaxPlayers] = {};
    static int s_frame[Plugin::kMaxPlayers] = {};
    if (slot < 0 || slot >= Plugin::kMaxPlayers) return;
    s_frame[slot]++;
    const bool burst = s_count[slot] < kBurstLines;
    const int interval =
        s_count[slot] < kEarlyLines ? kEarlyIntervalFrames : kSteadyIntervalFrames;
    if (!burst && (s_frame[slot] % interval) != 0) return;
    s_count[slot]++;

    const float* org = view.Origin();
    const float* ang = view.Angles();
    HT_LOG("[view] p%d rect=(%d,%d %dx%d) org=(%.1f,%.1f,%.1f) ang=(p%.2f y%.2f r%.2f) "
           "fov=%.2f/%.2f | track=%d delta=(p%.2f y%.2f r%.2f) pos=(%.2f,%.2f,%.2f)",
           slot + 1, view.RectX(), view.RectY(), view.RectWidth(), view.RectHeight(),
           org[0], org[1], org[2], ang[0], ang[1], ang[2], view.Fov(), view.FovViewmodel(),
           delta.applied ? 1 : 0, delta.pitch, delta.yaw, delta.roll,
           delta.x, delta.y, delta.z);
}

// ----- The hook -------------------------------------------------------------

// The tracking work, separated from the detour so the original call can sit
// outside the try. Nothing here is expected to throw, but the pipeline touches
// std::string and std::function, and an exception unwinding out of a __fastcall
// detour through a MinHook trampoline into client.dll frames would skip the
// original RenderView - a black screen, then terminate. Swallowing is the one
// correct answer here: a dropped frame of head tracking is nothing, a frame the
// engine never renders is everything.
void ApplyTracking(const ViewSetup& view) {
    Plugin& plugin = GetPlugin();
    const int slot = SplitScreenSlot(view, plugin.ActivePlayers());
    plugin.Update(slot);  // pulls this viewport's fresh tracker sample

    // Not gated on tracker data - the FOV is a view setting, not a pose, and a
    // player whose tracker is asleep still wants the frame they configured. It
    // IS gated on the tracking toggle, so End leaves a completely vanilla view
    // behind rather than a vanilla view at a modded FOV.
    if (plugin.IsEnabled()) {
        const Config& config = plugin.GetConfig();
        ApplyFovOverride(view, slot, config.fov_override, config.fov_viewmodel_override);
    }

    float* ang = view.Angles();

    AimState aim;
    for (int i = 0; i < 3; ++i) aim.clean[i] = ang[i];

    TrackingDelta delta;
    float yaw_r, pitch_r, roll_r;
    if (plugin.GetRotationRadians(slot, yaw_r, pitch_r, roll_r)) {
        delta.applied = true;
        float* org = view.Origin();

        // Clean basis (before rotation) for body-relative position translation.
        float fwd[3], right[3], up[3];
        source::AngleVectors(ang, fwd, right, up);

        // Positional 6DOF: shift the render origin in the clean view basis so
        // the lean follows the body, not the head-rotated view.
        if (plugin.GetPositionOffset(slot, delta.x, delta.y, delta.z)) {
            delta.x *= kPosXSign;
            delta.y *= kPosYSign;
            delta.z *= kPosZSign;
            for (int i = 0; i < 3; ++i) {
                org[i] += right[i] * delta.x + up[i] * delta.y + fwd[i] * delta.z;
            }
        }

        delta.pitch = pitch_r * kRadToDeg * kPitchSign;
        delta.yaw   = yaw_r   * kRadToDeg * kYawSign;
        delta.roll  = roll_r  * kRadToDeg * kRollSign;

        if (plugin.IsWorldSpaceYaw()) {
            // Source QAngle is intrinsically horizon-locked - yaw is about
            // world up, pitch about the yawed right axis - so adding the head
            // delta straight on IS the world-space-yaw composition.
            ang[0] += delta.pitch;
            ang[1] += delta.yaw;
            ang[2] += delta.roll;
        } else {
            source::ApplyCameraLocalRotation(ang, delta.pitch, delta.yaw, delta.roll);
        }
    }

    // Published unconditionally, including the untracked case: a stale delta
    // left behind after tracking stops would hold the reticle off-centre with
    // nothing moving the view any more.
    aim.applied = delta.applied;
    for (int i = 0; i < 3; ++i) aim.tracked[i] = ang[i];
    aim.fov = view.Fov();
    aim.width = view.RectWidth();
    aim.height = view.RectHeight();
    PublishAimState(slot, aim);

    DiagnosticLog(slot, view, delta);
}

void __fastcall Hook_RenderView(void* ecx, void* edx, void* view, void* hud,
                                int clearFlags, int whatToDraw) {
    if (view) {
        try {
            ApplyTracking(ViewSetup(view, g_profile->offsets.view_setup));
        } catch (...) {
            // Deliberately silent: logging from here could throw again, and the
            // only thing that matters is reaching the original call below.
        }
    }

    g_originalRenderView(ecx, edx, view, hud, clearFlags, whatToDraw);
}

// ----- Installation ---------------------------------------------------------

// client.dll is loaded long after the ASI, so the bootstrap thread waits for it
// rather than giving up on the first miss.
constexpr int   kClientWaitAttempts   = 200;
constexpr DWORD kClientWaitIntervalMs = 100;

HMODULE WaitForClientModule() {
    for (int i = 0; i < kClientWaitAttempts; ++i) {
        if (HMODULE client = GetModuleHandleA("client.dll")) return client;
        Sleep(kClientWaitIntervalMs);
    }
    return nullptr;
}

// Fingerprints the running client.dll and returns its profile, or nullptr -
// which is the dormant path: the game runs vanilla and the log says why.
const builds::BuildProfile* ResolveBuildProfile(HMODULE client) {
    cameraunlock::memory::PeFingerprint fp{};
    if (!cameraunlock::memory::ReadPeFingerprint(client, fp)) {
        HT_LOG("[hook] could not read client.dll fingerprint");
        return nullptr;
    }
    HT_LOG("[hook] client.dll fingerprint TimeDateStamp=0x%08X SizeOfImage=0x%08X CheckSum=0x%08X",
           fp.TimeDateStamp, fp.SizeOfImage, fp.CheckSum);

    const builds::BuildProfile* profile = builds::MatchProfile(fp);
    if (!profile) {
        builds::LogUnrecognisedBuild(fp);
        return nullptr;
    }
    if (!profile->IsComplete()) {
        HT_LOG("[hook] build profile '%s' is a placeholder (hook target not yet rederived) "
               "- staying dormant", profile->name);
        return nullptr;
    }
    HT_LOG("[hook] matched build profile '%s'", profile->name);
    return profile;
}

bool InstallRenderViewDetour(void* target) {
    using cameraunlock::hooks::HookManager;
    using cameraunlock::hooks::HookStatus;

    if (HookManager::Instance().Initialize() != HookStatus::Ok) {
        HT_LOG("[hook] MinHook init failed");
        return false;
    }
    const HookStatus created = HookManager::Instance().CreateHook(
        target, reinterpret_cast<void*>(&Hook_RenderView),
        reinterpret_cast<void**>(&g_originalRenderView));
    if (created != HookStatus::Ok) {
        HT_LOG("[hook] CreateHook(RenderView) failed: %s",
               cameraunlock::hooks::HookStatusToString(created));
        return false;
    }
    if (HookManager::Instance().EnableHook(target) != HookStatus::Ok) {
        HT_LOG("[hook] EnableHook(RenderView) failed");
        return false;
    }
    HT_LOG("[hook] RenderView hook installed at %p", target);
    return true;
}

}  // namespace

bool CameraHook::Install() {
    HMODULE client = WaitForClientModule();
    if (!client) {
        HT_LOG("[hook] client.dll never loaded");
        return false;
    }

    // Published before the detour is armed: the very first RenderView can land
    // inside EnableHook, and it dereferences this.
    g_profile = ResolveBuildProfile(client);
    if (!g_profile) return false;

    void* target = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(client)
                                           + g_profile->offsets.render_view_rva);
    return InstallRenderViewDetour(target);
}

}  // namespace headtracking
