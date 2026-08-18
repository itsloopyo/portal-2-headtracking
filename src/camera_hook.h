#pragma once

namespace headtracking {

// Installs a MinHook detour on the Source client's render view-setup function
// (client.dll). The detour injects the head pose into the CViewSetup the
// renderer consumes - the render view origin + angles only.
//
// What stays clean is the game's own camera state: cl.viewangles, which aim,
// portal placement and raycasts read. That is what decouples look from aim,
// and it is what `getpos` confirms in .lab/NOTES.md. Render-phase globals that
// RenderView itself seeds from this struct (MainViewOrigin/MainViewForward and
// friends) DO see the tracked pose - they are the render view, so sprites,
// beams and audio panning following the head is the correct behaviour, not a
// leak.
//
// The hook is gated on a PE-fingerprint build-profile registry: it engages
// only on a Portal 2 client.dll build it has offsets for, and stays dormant
// (game runs vanilla) on any other build. See builds/build_registry.h.
//
// Once installed it is never removed. There is no Uninstall: unhooking means
// freeing a trampoline a render thread may be about to jump into, and the only
// caller would be a DLL_PROCESS_DETACH path that must not do work anyway (see
// dllmain.cpp). The process teardown reclaims it.
class CameraHook {
public:
    CameraHook() = default;

    bool Install();
};

}  // namespace headtracking
