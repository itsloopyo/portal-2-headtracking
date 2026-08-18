#pragma once

#include <cmath>

#include "angles.h"
#include "source_math.h"

namespace headtracking {

// Where the clean aim direction lands in the head-tracked view, as a pixel
// offset from the centre of the viewport.
//
// Portal 2 draws its reticle at hard screen centre - no projection of any kind
// - so unlike a Source game that routes the crosshair through
// CHudCrosshair::GetDrawPosition, there is no engine projection to feed an
// angle offset into. The offset has to be computed here and applied to the
// draw position.
//
// It is derived from the two angle triples rather than per-axis tangents: the
// clean aim direction is expressed in the tracked view's own basis and then
// perspective divided. That is correct for any combination of yaw, pitch and
// roll without assuming a composition order, which matters because the naive
// `tan(yaw)/tan(fov/2)` form is roll-unaware and drifts horizontally as soon as
// the head tilts while pitched. Both bases come from the same AngleVectors the
// camera hook composes the view with, so the projection cannot fall out of step
// with what the frame actually renders.
//
// Returns false when the aim point is not in front of the tracked view, which
// happens on a large head turn; the caller hides the reticle rather than
// drawing it mirrored behind the camera.
//
// Known limitation: the ray is projected from the tracked eye, not the clean
// one, so 6DOF lean leaves a parallax error. Removing it needs the aim point's
// distance, i.e. a trace into the world. The error is proportional to
// lean/distance and so is negligible at the ranges a portal is usually placed.
inline bool ComputeReticleOffset(const float clean[3], const float tracked[3],
                                 float fovDegrees, int width, int height,
                                 float& dx, float& dy) {
    if (width <= 0 || height <= 0 || fovDegrees <= 0.0f || fovDegrees >= 180.0f) return false;

    float fwd[3], right[3], up[3];
    source::AngleVectors(tracked, fwd, right, up);

    float aim[3], aimRight[3], aimUp[3];
    source::AngleVectors(clean, aim, aimRight, aimUp);

    const float vz = aim[0] * fwd[0] + aim[1] * fwd[1] + aim[2] * fwd[2];
    if (vz <= 0.01f) return false;

    const float vx = aim[0] * right[0] + aim[1] * right[1] + aim[2] * right[2];
    const float vy = aim[0] * up[0] + aim[1] * up[1] + aim[2] * up[2];

    // fovDegrees is the horizontal FOV the frame is rendered with - the value
    // already widened for this viewport's aspect, which is what CViewSetup
    // carries by the time the render hook sees it.
    const float tanHalfH = std::tan(fovDegrees * 0.5f * kDegToRad);
    const float tanHalfV = tanHalfH * static_cast<float>(height) / static_cast<float>(width);

    dx = (vx / vz) / tanHalfH * (static_cast<float>(width) * 0.5f);
    dy = -(vy / vz) / tanHalfV * (static_cast<float>(height) * 0.5f);
    return true;
}

}  // namespace headtracking
