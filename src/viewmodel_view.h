// SPDX-License-Identifier: MIT
// Copyright (c) 2026 itsloopyo / CameraUnlock
#pragma once

#include <cmath>

#include "angles.h"
#include "source_math.h"

namespace headtracking {

// The angles to draw the portal gun's pass with, so the gun stays on the
// reticle under head tracking.
//
// The gun is drawn in a second pass that reuses the world pass's view and swaps
// in CViewSetup::fovViewmodel, which is far narrower (58.46 against 100.39 in a
// 1280x800 window). The gun model itself sits on the CLEAN aim axis, because
// the game places it from its own camera, so under head tracking there is a
// real angle between that axis and the view the frame is drawn from, and the
// narrower projection magnifies it by tan(fov/2) / tan(fovViewmodel/2): about
// 2.1x at those numbers. The gun swings across the frame twice as far as the
// world does and leaves the reticle behind at once.
//
// The correction expresses the clean aim axis in the tracked basis, scales its
// two lateral components by tan(fovViewmodel/2) / tan(fov/2), and turns the
// tracked basis by the shortest arc that carries the scaled direction back onto
// the aim axis. Seen through the viewmodel projection from the turned basis,
// the aim axis lands on the pixel the world projection puts it on. The shortest
// arc adds no spin of its own, so head roll carries through untouched, and with
// equal FOVs it is the identity.
//
// The two FOVs are the fields DrawViewModels builds both projections from, both
// horizontal degrees already widened for this viewport, and both projections
// use the same aspect. So their tangent ratio IS the ratio of the two
// projections' scale terms, with no unit or axis conversion in between.
//
// Basis to basis rather than scaling the head's yaw and pitch: per-axis Euler
// scaling agrees on a single axis and drifts on any combined pose.
//
// Returns false, leaving `out` untouched, for a FOV that has no projection.
inline bool ComputeViewmodelAngles(const float clean[3], const float tracked[3], float fov,
                                   float fovViewmodel, float out[3]) {
    if (!(fov > 0.0f && fov < 180.0f && fovViewmodel > 0.0f && fovViewmodel < 180.0f)) {
        return false;
    }

    float fwd[3], right[3], up[3];
    source::AngleVectors(tracked, fwd, right, up);
    float aim[3], aimRight[3], aimUp[3];
    source::AngleVectors(clean, aim, aimRight, aimUp);

    const auto dot = [](const float* a, const float* b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };

    // Camera-local coordinates, (forward, right, up).
    const float u[3] = { dot(aim, fwd), dot(aim, right), dot(aim, up) };

    const float k = std::tan(fovViewmodel * 0.5f * kDegToRad) / std::tan(fov * 0.5f * kDegToRad);
    float s[3] = { u[0], k * u[1], k * u[2] };
    const float len = std::sqrt(dot(s, s));
    for (float& c : s) c /= len;

    // Rotation taking s onto u: Qv = v*d + c x v + c*(c.v)/(1+d), with c = s x u
    // and d = s.u. Scaling by a positive k keeps s in u's half-space, so d > -1.
    const float c[3] = { s[1] * u[2] - s[2] * u[1],
                         s[2] * u[0] - s[0] * u[2],
                         s[0] * u[1] - s[1] * u[0] };
    const float d = dot(s, u);
    const auto rotate = [&](const float v[3], float r[3]) {
        const float cv = dot(c, v) / (1.0f + d);
        r[0] = v[0] * d + (c[1] * v[2] - c[2] * v[1]) + c[0] * cv;
        r[1] = v[1] * d + (c[2] * v[0] - c[0] * v[2]) + c[1] * cv;
        r[2] = v[2] * d + (c[0] * v[1] - c[1] * v[0]) + c[2] * cv;
    };

    static constexpr float kAxes[3][3] = { {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} };
    float world[3][3];
    for (int axis = 0; axis < 3; ++axis) {
        float q[3];
        rotate(kAxes[axis], q);
        for (int i = 0; i < 3; ++i) {
            world[axis][i] = q[0] * fwd[i] + q[1] * right[i] + q[2] * up[i];
        }
    }

    const float left[3] = { -world[1][0], -world[1][1], -world[1][2] };
    source::BasisToAngles(world[0], left, world[2], out);
    return true;
}

}  // namespace headtracking
