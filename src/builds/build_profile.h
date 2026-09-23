// SPDX-License-Identifier: MIT
// Copyright (c) 2026 itsloopyo / CameraUnlock
#pragma once

#include <cstdint>

#include "cameraunlock/memory/pe_fingerprint.h"

namespace headtracking::builds {

// Byte offsets of the CViewSetup fields the render-view detour reads and
// writes. Source ships no headers, so these are rederived per build and pinned
// to that build's fingerprint - see the registry below.
struct ViewSetupOffsets {
    uint32_t origin;         // Vector origin
    uint32_t angles;         // QAngle angles (pitch, yaw, roll)
    uint32_t fov;            // float fov, horizontal degrees
    uint32_t fov_viewmodel;  // float fovViewmodel
    uint32_t rect_x;         // int x, viewport rect - split-screen tile origin
    uint32_t rect_y;         // int y
    uint32_t rect_width;     // int width
    uint32_t rect_height;    // int height
};

// Portal 2 draws its reticle at hard screen centre. Unlike a stock Source
// game it does NOT route the crosshair through CHudCrosshair::GetDrawPosition
// - that element exists and runs, but what it paints is invisible; the visible
// blue/orange arcs come from the portal gun's own HUD element, which positions
// all four pieces from width/2 and height/2 with no projection at all.
//
// The reticle is drawn by two elements between them: the portal gun's supplies
// the blue/orange arcs, CHudCrosshair the centre dots. So there is no engine
// offset to feed, and no single draw call to intercept either - the two
// elements do not even share a drawing primitive.
//
// What they DO share is where they start from: each asks for the screen size
// and positions its pieces around width/2, height/2. So compensation shifts
// that centre. Both Paints are detoured to mark the reticle as being drawn, and
// the screen-size getters are detoured to report a viewport whose centre sits on
// the projected aim point while either Paint is running.
//
// Shifting the origin rather than each piece is what makes this whole-reticle:
// it moves every element, in whatever way each chooses to draw, and it cannot
// desynchronise the parts from each other.
struct CrosshairOffsets {
    uint32_t reticle_paint_rva;         // portal gun element: the blue/orange arcs
    uint32_t hud_crosshair_paint_rva;   // CHudCrosshair: the centre dots
    uint32_t screen_width_rva;          // engine screen width, in pixels
    uint32_t screen_height_rva;         // engine screen height
};

// The portal gun is drawn in its own pass, CViewRender::DrawViewModels, which
// copies the render view, swaps in fovViewmodel and pushes the copy through the
// engine's IVRenderView::Push3DView. Giving that pass a view of its own needs
// both: DrawViewModels to know the pass is running, and Push3DView to reach the
// copy, which only exists inside it.
struct ViewmodelOffsets {
    uint32_t draw_view_models_rva;   // CViewRender::DrawViewModels
    uint32_t render_view_iface_rva;  // client.dll's IVRenderView* global
};

// The whole surface one client.dll build pins.
struct OffsetTable {
    uint32_t render_view_rva;  // CViewRender::RenderView, RVA in client.dll
    ViewSetupOffsets view_setup;
    CrosshairOffsets crosshair;
    ViewmodelOffsets viewmodel;
};

// One entry per shipped Portal 2 client.dll build we have offsets for. The PE
// fingerprint is the routing key.
struct BuildProfile {
    const char* name;
    cameraunlock::memory::PeFingerprint fingerprint;
    OffsetTable offsets;

    // A profile whose hook target is still unresolved is a placeholder: the
    // fingerprint of a build we have spotted but not yet rederived. It must
    // stay dormant rather than hook a stale address, so the entry can be
    // landed the moment a patch appears without risking a user's session.
    bool IsComplete() const { return offsets.render_view_rva != 0; }

    // Reticle compensation is a separate, optional surface: a profile can drive
    // the camera without it. A build whose crosshair addresses have not been
    // derived keeps head tracking and draws the vanilla centred crosshair.
    bool HasCrosshairOffsets() const {
        return offsets.crosshair.reticle_paint_rva != 0 &&
               offsets.crosshair.hud_crosshair_paint_rva != 0 &&
               offsets.crosshair.screen_width_rva != 0 &&
               offsets.crosshair.screen_height_rva != 0;
    }

    // Also optional. Without it the portal gun is drawn from the head-tracked
    // view at its own narrower FOV, which swings it off the reticle.
    bool HasViewmodelOffsets() const {
        return offsets.viewmodel.draw_view_models_rva != 0 &&
               offsets.viewmodel.render_view_iface_rva != 0;
    }
};

}  // namespace headtracking::builds
