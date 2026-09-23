// SPDX-License-Identifier: MIT
// Copyright (c) 2026 itsloopyo / CameraUnlock
// Tests for src/viewmodel_view.h - the view the portal gun's pass is drawn
// with, so the gun lands where the world projection puts the aim axis.
//
// The contract is one property: the clean aim axis, projected through the
// viewmodel FOV from the corrected angles, lands on the same screen point as it
// does through the world FOV from the tracked angles. Every pose below is
// checked against that, plus the closed form where one exists.

#include <cmath>
#include <cstdio>

#include "source_math.h"
#include "viewmodel_view.h"

namespace {

using headtracking::ComputeViewmodelAngles;

int g_failures = 0;

void Check(bool cond, const char* name) {
    std::printf(cond ? "  [PASS] %s\n" : "  [FAIL] %s\n", name);
    if (!cond) ++g_failures;
}

bool NearEqual(float a, float b, float eps) {
    return std::fabs(a - b) <= eps;
}

constexpr float kDeg = 3.14159265f / 180.0f;

// fov_desired 90 and the game's own viewmodel FOV, both widened for 1280x800 -
// the values the render view carries in game.
constexpr float kFov = 100.388855f;
constexpr float kFovViewmodel = 58.46f;

// Normalised screen position of `dir` seen from `angles` at `fov`: lateral over
// forward, over the half-FOV tangent. The vertical term shares the horizontal
// tangent because both passes use the same aspect.
void Project(const float dir[3], const float angles[3], float fov, float& x, float& y) {
    float f[3], r[3], u[3];
    headtracking::source::AngleVectors(angles, f, r, u);
    const float z = dir[0] * f[0] + dir[1] * f[1] + dir[2] * f[2];
    const float t = std::tan(fov * 0.5f * kDeg);
    x = (dir[0] * r[0] + dir[1] * r[1] + dir[2] * r[2]) / z / t;
    y = (dir[0] * u[0] + dir[1] * u[1] + dir[2] * u[2]) / z / t;
}

// The gun's aim axis lands on the same screen point in both passes.
void CheckLandsOnAimPoint(const float clean[3], const float tracked[3], const char* name) {
    float out[3];
    Check(ComputeViewmodelAngles(clean, tracked, kFov, kFovViewmodel, out), name);

    float aim[3], r[3], u[3];
    headtracking::source::AngleVectors(clean, aim, r, u);
    float wx, wy, vx, vy;
    Project(aim, tracked, kFov, wx, wy);
    Project(aim, out, kFovViewmodel, vx, vy);
    std::printf("    world (%.5f, %.5f)  viewmodel (%.5f, %.5f)\n", wx, wy, vx, vy);
    Check(NearEqual(wx, vx, 1e-4f) && NearEqual(wy, vy, 1e-4f),
          "aim axis lands on the same screen point in both passes");
}

void TestNoHeadMovement() {
    std::printf("viewmodel view: no head movement\n");
    const float ang[3] = {10.0f, 90.0f, 0.0f};
    float out[3];
    Check(ComputeViewmodelAngles(ang, ang, kFov, kFovViewmodel, out), "computes");
    Check(NearEqual(out[0], 10.0f, 1e-3f) && NearEqual(out[1], 90.0f, 1e-3f) &&
              NearEqual(out[2], 0.0f, 1e-3f),
          "clean == tracked leaves the view alone");
}

void TestEqualFovsIsIdentity() {
    std::printf("viewmodel view: equal FOVs\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {12.0f, 70.0f, 8.0f};
    float out[3];
    Check(ComputeViewmodelAngles(clean, tracked, kFov, kFov, out), "computes");
    Check(NearEqual(out[0], 12.0f, 1e-3f) && NearEqual(out[1], 70.0f, 1e-3f) &&
              NearEqual(out[2], 8.0f, 1e-3f),
          "no magnification means the tracked view is already right");
}

void TestPureYaw() {
    std::printf("viewmodel view: pure yaw\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {0.0f, 75.0f, 0.0f};  // head turned 15 degrees
    float out[3];
    Check(ComputeViewmodelAngles(clean, tracked, kFov, kFovViewmodel, out), "computes");
    // The aim axis sits atan(k * tan 15) off the corrected view's centre.
    const float k = std::tan(kFovViewmodel * 0.5f * kDeg) / std::tan(kFov * 0.5f * kDeg);
    const float expectedYaw = 90.0f - std::atan(k * std::tan(15.0f * kDeg)) / kDeg;
    std::printf("    out (%.4f, %.4f, %.4f), expected yaw %.4f\n", out[0], out[1], out[2],
                expectedYaw);
    Check(NearEqual(out[1], expectedYaw, 1e-3f), "yaw matches the closed form");
    Check(NearEqual(out[0], 0.0f, 1e-3f) && NearEqual(out[2], 0.0f, 1e-3f),
          "pure yaw adds no pitch or roll");
    CheckLandsOnAimPoint(clean, tracked, "pure yaw");
}

void TestPurePitch() {
    std::printf("viewmodel view: pure pitch\n");
    const float clean[3]   = {5.0f, 90.0f, 0.0f};
    const float tracked[3] = {-15.0f, 90.0f, 0.0f};
    CheckLandsOnAimPoint(clean, tracked, "pure pitch");
}

void TestRollAlone() {
    std::printf("viewmodel view: pure roll\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {0.0f, 90.0f, 25.0f};
    float out[3];
    Check(ComputeViewmodelAngles(clean, tracked, kFov, kFovViewmodel, out), "computes");
    Check(NearEqual(out[0], 0.0f, 1e-3f) && NearEqual(out[1], 90.0f, 1e-3f) &&
              NearEqual(out[2], 25.0f, 1e-3f),
          "roll about the aim axis is carried through unchanged");
}

void TestCombinedPose() {
    std::printf("viewmodel view: combined yaw + pitch + roll\n");
    const float clean[3]   = {20.0f, 90.0f, 0.0f};
    const float tracked[3] = {5.0f, 68.0f, -30.0f};
    CheckLandsOnAimPoint(clean, tracked, "combined pose");
}

void TestSteepCleanPitch() {
    std::printf("viewmodel view: game camera looking steeply down\n");
    const float clean[3]   = {80.0f, 45.0f, 0.0f};
    const float tracked[3] = {70.0f, 20.0f, 10.0f};
    CheckLandsOnAimPoint(clean, tracked, "steep pitch");
}

void TestRejectsDegenerateFov() {
    std::printf("viewmodel view: degenerate FOV\n");
    const float ang[3] = {0.0f, 0.0f, 0.0f};
    float out[3] = {1.0f, 2.0f, 3.0f};
    Check(!ComputeViewmodelAngles(ang, ang, 0.0f, kFovViewmodel, out), "world fov 0 refused");
    Check(!ComputeViewmodelAngles(ang, ang, kFov, 180.0f, out), "viewmodel fov 180 refused");
    Check(out[0] == 1.0f && out[1] == 2.0f && out[2] == 3.0f, "out left untouched");
}

}  // namespace

int RunViewmodelViewTests() {
    std::printf("\nViewmodel view\n--------------\n");
    TestNoHeadMovement();
    TestEqualFovsIsIdentity();
    TestPureYaw();
    TestPurePitch();
    TestRollAlone();
    TestCombinedPose();
    TestSteepCleanPitch();
    TestRejectsDegenerateFov();
    return g_failures;
}
