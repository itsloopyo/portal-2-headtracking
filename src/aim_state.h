#pragma once

namespace headtracking {

// The bridge between the two render-phase detours. RenderView writes the head
// pose into the CViewSetup the frame is drawn from; the crosshair is painted
// later in that same call and has no view of its own to read, so it needs the
// angles the game aims along and the angles the frame actually renders with.
// Their difference is what puts the reticle back on the aim point - see
// crosshair_hook.cpp.
//
// Render-thread only, and deliberately unsynchronised: the HUD paint that
// consumes this happens inside the RenderView call that publishes it, on the
// same thread. An atomic here would buy a fence on the hot path and nothing
// else.
struct AimState {
    bool  applied = false;                  // tracking modified this viewport
    float clean[3]   = {0.0f, 0.0f, 0.0f};  // QAngle the game aims along
    float tracked[3] = {0.0f, 0.0f, 0.0f};  // QAngle the frame renders with
    // The viewport the projection has to land in. Taken from the same
    // CViewSetup, so an FOV override is already folded into fov.
    float fov = 0.0f;
    int   width = 0;
    int   height = 0;
};

// Called once per viewport per frame by the render-view detour. Also marks this
// viewport as the one a following crosshair paint belongs to: in split-screen,
// CViewRender::Render renders and draws each player's HUD in turn.
void PublishAimState(int slot, const AimState& state);

// The viewport most recently published, i.e. the one being painted now.
const AimState& CurrentAimState();

}  // namespace headtracking
