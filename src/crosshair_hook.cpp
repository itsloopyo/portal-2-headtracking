#include "crosshair_hook.h"

#include <Windows.h>
#include <cstdint>

#include "aim_state.h"
#include "builds/build_registry.h"
#include "cameraunlock/hooks/hook_manager.h"
#include "debug_log.h"
#include "reticle_projection.h"

namespace headtracking {

namespace {

using PaintFn = void(__fastcall*)(void* ecx, void* edx);
using ScreenDimFn = int(__cdecl*)();

PaintFn g_originalReticlePaint = nullptr;
PaintFn g_originalHudCrosshairPaint = nullptr;
ScreenDimFn g_originalScreenWidth = nullptr;
ScreenDimFn g_originalScreenHeight = nullptr;

// Armed for exactly the span of a reticle Paint. The screen size is asked for
// all over the HUD, so without this scoping every other element would move too.
bool  g_paintingReticle = false;
bool  g_offsetValid = false;
float g_offsetX = 0.0f;
float g_offsetY = 0.0f;

int RoundToInt(float v) {
    return static_cast<int>(v < 0.0f ? v - 0.5f : v + 0.5f);
}

// Both reticle elements derive their position from half the screen size, so
// reporting a viewport 2*offset wider moves the centre they compute by exactly
// offset - and with it every piece they draw, however each of them draws it.
// The doubling is what makes their halving land on the right pixel.
int __cdecl Hook_ScreenWidth() {
    const int w = g_originalScreenWidth();
    if (!g_paintingReticle || !g_offsetValid) return w;
    return w + 2 * RoundToInt(g_offsetX);
}

int __cdecl Hook_ScreenHeight() {
    const int h = g_originalScreenHeight();
    if (!g_paintingReticle || !g_offsetValid) return h;
    return h + 2 * RoundToInt(g_offsetY);
}

void PaintWithOffsetReticle(void* ecx, void* edx, PaintFn original) {
    const AimState& aim = CurrentAimState();

    g_offsetValid = aim.applied &&
                    ComputeReticleOffset(aim.clean, aim.tracked, aim.fov, aim.width, aim.height,
                                         g_offsetX, g_offsetY);

    // Aim point behind the tracked view, which a large head turn reaches: no
    // screen position means "the portal lands here", so drawing nothing beats
    // drawing the reticle somewhere that lies.
    if (aim.applied && !g_offsetValid) return;

    g_paintingReticle = true;
    original(ecx, edx);
    g_paintingReticle = false;
}

void __fastcall Hook_ReticlePaint(void* ecx, void* edx) {
    PaintWithOffsetReticle(ecx, edx, g_originalReticlePaint);
}

void __fastcall Hook_HudCrosshairPaint(void* ecx, void* edx) {
    PaintWithOffsetReticle(ecx, edx, g_originalHudCrosshairPaint);
}

bool InstallDetour(void* target, void* detour, void** original, const char* what) {
    using cameraunlock::hooks::HookManager;
    using cameraunlock::hooks::HookStatus;

    const HookStatus created = HookManager::Instance().CreateHook(target, detour, original);
    if (created != HookStatus::Ok) {
        HT_LOG("[crosshair] CreateHook(%s) failed: %s", what,
               cameraunlock::hooks::HookStatusToString(created));
        return false;
    }
    if (HookManager::Instance().EnableHook(target) != HookStatus::Ok) {
        HT_LOG("[crosshair] EnableHook(%s) failed", what);
        return false;
    }
    HT_LOG("[crosshair] %s hook installed at %p", what, target);
    return true;
}

}  // namespace

bool CrosshairHook::Install() {
    const builds::BuildProfile* profile = builds::ActiveProfile();
    if (!profile) return false;

    if (!profile->HasCrosshairOffsets()) {
        HT_LOG("[crosshair] build profile '%s' has no reticle addresses - the reticle stays "
               "centred (head tracking is unaffected)", profile->name);
        return false;
    }

    HMODULE client = GetModuleHandleA("client.dll");
    if (!client) {
        HT_LOG("[crosshair] client.dll not loaded");
        return false;
    }

    const builds::CrosshairOffsets& off = profile->offsets.crosshair;
    const auto ptr = [client](uint32_t rva) {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(client) + rva);
    };

    // All four or none. The screen-size detours are inert until a Paint detour
    // arms them, and a Paint detour with no screen-size detour moves nothing, so
    // half of this installed is a mod that looks active and is not.
    const bool ok =
        InstallDetour(ptr(off.screen_width_rva), reinterpret_cast<void*>(&Hook_ScreenWidth),
                      reinterpret_cast<void**>(&g_originalScreenWidth), "screen width") &&
        InstallDetour(ptr(off.screen_height_rva), reinterpret_cast<void*>(&Hook_ScreenHeight),
                      reinterpret_cast<void**>(&g_originalScreenHeight), "screen height") &&
        InstallDetour(ptr(off.reticle_paint_rva), reinterpret_cast<void*>(&Hook_ReticlePaint),
                      reinterpret_cast<void**>(&g_originalReticlePaint), "portal reticle Paint") &&
        InstallDetour(ptr(off.hud_crosshair_paint_rva),
                      reinterpret_cast<void*>(&Hook_HudCrosshairPaint),
                      reinterpret_cast<void**>(&g_originalHudCrosshairPaint),
                      "CHudCrosshair::Paint");
    if (!ok) {
        HT_LOG("[crosshair] reticle compensation unavailable - the reticle stays centred");
    }
    return ok;
}

}  // namespace headtracking
