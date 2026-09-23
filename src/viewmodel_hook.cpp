// SPDX-License-Identifier: MIT
// Copyright (c) 2026 itsloopyo / CameraUnlock
#include "viewmodel_hook.h"

#include <Windows.h>
#include <cstdint>

#include "aim_state.h"
#include "builds/build_registry.h"
#include "cameraunlock/hooks/hook_manager.h"
#include "debug_log.h"
#include "view_setup.h"
#include "viewmodel_view.h"

namespace headtracking {

namespace {

// CViewRender::DrawViewModels(const CViewSetup& view, bool drawViewmodel).
// The bool is pushed widened to a dword.
using DrawViewModelsFn = void(__fastcall*)(void* ecx, void* edx, void* view, int drawViewmodel);

// IVRenderView::Push3DView(IMatRenderContext*, const CViewSetup&, int flags,
// ITexture* renderTarget, Frustum frustumPlanes).
using Push3DViewFn = void(__fastcall*)(void* ecx, void* edx, void* ctx, void* setup, int flags,
                                       void* renderTarget, void* frustum);
constexpr size_t kPush3DViewVtableIndex = 0xA0 / sizeof(void*);

DrawViewModelsFn g_originalDrawViewModels = nullptr;
Push3DViewFn g_originalPush3DView = nullptr;
const builds::ViewSetupOffsets* g_layout = nullptr;

// Armed for exactly the span of a DrawViewModels call. Push3DView is how every
// view in the game is drawn, so outside that span it must pass straight
// through.
//
// DrawViewModels pushes one copy of the render view twice: first at
// fovViewmodel for the gun, then, when there is anything else in the viewmodel
// group, again at the world FOV. Only the first is the gun, and the second has
// to be put back to the tracked view the copy started as, because it draws
// objects that belong to the world.
bool  g_armed = false;
int   g_pushIndex = 0;
float g_gunAngles[3];
float g_gunOrigin[3];
float g_trackedAngles[3];
float g_trackedOrigin[3];

bool Arm(const ViewSetup& view) {
    const AimState& aim = CurrentAimState();
    if (!aim.applied) return false;

    const float* tracked = view.Angles();
    if (!ComputeViewmodelAngles(aim.clean, tracked, view.Fov(), view.FovViewmodel(), g_gunAngles)) {
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            HT_LOG("[viewmodel] fov %.2f / viewmodel fov %.2f has no projection - the portal "
                   "gun is drawn as the game draws it", view.Fov(), view.FovViewmodel());
        }
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        g_gunOrigin[i] = aim.cleanOrigin[i];
        g_trackedAngles[i] = tracked[i];
        g_trackedOrigin[i] = view.Origin()[i];
    }
    return true;
}

void Write(const ViewSetup& setup, const float angles[3], const float origin[3]) {
    for (int i = 0; i < 3; ++i) {
        setup.Angles()[i] = angles[i];
        setup.Origin()[i] = origin[i];
    }
}

// The numbers the gun pass is built from, so a report of the gun sitting off
// the reticle arrives with the answer in the log: the pass being corrected
// should carry the viewmodel FOV, and the gun angles should sit between clean
// and tracked. Throttled to the same steady cadence as the [view] line rather
// than capped, so a late fault is still on record.
constexpr int kGunPassLogInterval = 2000;

void LogGunPass(const ViewSetup& setup) {
    static int s_passes = 0;
    if (s_passes++ % kGunPassLogInterval != 0) return;
    const AimState& aim = CurrentAimState();
    HT_LOG("[viewmodel] gun pass fov=%.2f (world %.2f) clean=(p%.2f y%.2f r%.2f) "
           "tracked=(p%.2f y%.2f r%.2f) gun=(p%.2f y%.2f r%.2f)",
           setup.Fov(), aim.fov, aim.clean[0], aim.clean[1], aim.clean[2],
           g_trackedAngles[0], g_trackedAngles[1], g_trackedAngles[2],
           g_gunAngles[0], g_gunAngles[1], g_gunAngles[2]);
}

void __fastcall Hook_Push3DView(void* ecx, void* edx, void* ctx, void* setup, int flags,
                                void* renderTarget, void* frustum) {
    if (g_armed && setup) {
        const ViewSetup view(setup, *g_layout);
        if (g_pushIndex++ == 0) {
            Write(view, g_gunAngles, g_gunOrigin);
            LogGunPass(view);
        } else {
            Write(view, g_trackedAngles, g_trackedOrigin);
        }
    }
    g_originalPush3DView(ecx, edx, ctx, setup, flags, renderTarget, frustum);
}

void __fastcall Hook_DrawViewModels(void* ecx, void* edx, void* view, int drawViewmodel) {
    g_armed = view && Arm(ViewSetup(view, *g_layout));
    g_pushIndex = 0;
    g_originalDrawViewModels(ecx, edx, view, drawViewmodel);
    g_armed = false;
}

bool InstallDetour(void* target, void* detour, void** original, const char* what) {
    using cameraunlock::hooks::HookManager;
    using cameraunlock::hooks::HookStatus;

    const HookStatus created = HookManager::Instance().CreateHook(target, detour, original);
    if (created != HookStatus::Ok) {
        HT_LOG("[viewmodel] CreateHook(%s) failed: %s", what,
               cameraunlock::hooks::HookStatusToString(created));
        return false;
    }
    if (HookManager::Instance().EnableHook(target) != HookStatus::Ok) {
        HT_LOG("[viewmodel] EnableHook(%s) failed", what);
        return false;
    }
    HT_LOG("[viewmodel] %s hook installed at %p", what, target);
    return true;
}

// client.dll fills its IVRenderView pointer during its own Init, which can run
// after the bootstrap thread gets here.
constexpr int   kInterfaceWaitAttempts   = 200;
constexpr DWORD kInterfaceWaitIntervalMs = 100;

void* WaitForRenderViewInterface(void* const* slot) {
    for (int i = 0; i < kInterfaceWaitAttempts; ++i) {
        if (void* iface = *slot) return iface;
        Sleep(kInterfaceWaitIntervalMs);
    }
    return nullptr;
}

}  // namespace

bool ViewmodelHook::Install() {
    const builds::BuildProfile* profile = builds::ActiveProfile();
    if (!profile) return false;

    if (!profile->HasViewmodelOffsets()) {
        HT_LOG("[viewmodel] build profile '%s' has no viewmodel addresses - the portal gun "
               "is drawn as the game draws it (head tracking is unaffected)", profile->name);
        return false;
    }

    HMODULE client = GetModuleHandleA("client.dll");
    if (!client) {
        HT_LOG("[viewmodel] client.dll not loaded");
        return false;
    }
    const auto ptr = [client](uint32_t rva) {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(client) + rva);
    };
    const builds::ViewmodelOffsets& off = profile->offsets.viewmodel;
    g_layout = &profile->offsets.view_setup;

    void* renderView = WaitForRenderViewInterface(
        static_cast<void* const*>(ptr(off.render_view_iface_rva)));
    if (!renderView) {
        HT_LOG("[viewmodel] client.dll never set its IVRenderView pointer - the portal gun "
               "is drawn as the game draws it");
        return false;
    }
    void* push3DView = (*static_cast<void* const* const*>(renderView))[kPush3DViewVtableIndex];

    const bool ok =
        InstallDetour(push3DView, reinterpret_cast<void*>(&Hook_Push3DView),
                      reinterpret_cast<void**>(&g_originalPush3DView),
                      "IVRenderView::Push3DView") &&
        InstallDetour(ptr(off.draw_view_models_rva), reinterpret_cast<void*>(&Hook_DrawViewModels),
                      reinterpret_cast<void**>(&g_originalDrawViewModels),
                      "CViewRender::DrawViewModels");
    if (!ok) {
        HT_LOG("[viewmodel] portal gun compensation unavailable - the gun is drawn as the "
               "game draws it");
    }
    return ok;
}

}  // namespace headtracking
