// Tests for src/reticle_projection.h - where the clean aim direction lands on
// screen once the head has moved the view away from it.
//
// This one is not purely a characterization suite. Portal 2 draws its reticle
// at hard screen centre with no projection of its own (see .lab/NOTES.md), so
// unlike the camera maths there is no engine behaviour to mirror: the offset is
// ours to derive, and these check it against closed-form geometry.
//
// The single-axis expectations are hand-computable and were confirmed in-game:
// a 25 degree yaw at fov 100.39 in a 1280x800 viewport moved the reticle
// -248.7px, and the mod logged -248.7.

#include <cmath>
#include <cstdio>

#include "reticle_projection.h"

namespace {

using headtracking::ComputeReticleOffset;

int g_failures = 0;

void Check(bool cond, const char* name) {
    std::printf(cond ? "  [PASS] %s\n" : "  [FAIL] %s\n", name);
    if (!cond) ++g_failures;
}

bool NearEqual(float a, float b, float eps = 0.5f) {
    return std::fabs(a - b) <= eps;
}

// The viewport the in-game verification ran at: fov_desired 90 widened to
// 100.39 for a 1280x800 window.
constexpr float kFov = 100.388855f;
constexpr int kW = 1280;
constexpr int kH = 800;

// Closed form for a single-axis offset: the tangent of the angle over the
// tangent of the half-FOV, scaled to half the viewport.
float ExpectedX(float degrees) {
    const float k = std::tan(degrees * 3.14159265f / 180.0f)
                    / std::tan(kFov * 0.5f * 3.14159265f / 180.0f);
    return k * (kW * 0.5f);
}

void TestCentredWhenNotTracking() {
    std::printf("reticle offset: no head movement\n");
    const float clean[3] = {0.0f, 90.0f, 0.0f};
    float dx = 999.0f, dy = 999.0f;
    Check(ComputeReticleOffset(clean, clean, kFov, kW, kH, dx, dy), "identical angles project");
    Check(NearEqual(dx, 0.0f, 0.01f) && NearEqual(dy, 0.0f, 0.01f),
          "clean == tracked leaves the reticle at centre");
}

void TestPureYaw() {
    std::printf("reticle offset: pure yaw\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {0.0f, 65.0f, 0.0f};  // head turned 25 degrees
    float dx = 0.0f, dy = 0.0f;
    Check(ComputeReticleOffset(clean, tracked, kFov, kW, kH, dx, dy), "projects");
    // The value the mod logged and the screenshot confirmed.
    Check(NearEqual(dx, -248.7f), "25 degree yaw offsets -248.7px");
    Check(NearEqual(dx, -ExpectedX(25.0f)), "matches the closed form");
    Check(NearEqual(dy, 0.0f, 0.01f), "pure yaw does not move the reticle vertically");
}

void TestPurePitch() {
    std::printf("reticle offset: pure pitch\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {-20.0f, 90.0f, 0.0f};  // head looked up 20 degrees
    float dx = 0.0f, dy = 0.0f;
    Check(ComputeReticleOffset(clean, tracked, kFov, kW, kH, dx, dy), "projects");
    Check(NearEqual(dy, 194.1f), "20 degree pitch offsets +194.1px");
    Check(NearEqual(dx, 0.0f, 0.01f), "pure pitch does not move the reticle horizontally");
}

// The reason this projection goes through the two bases rather than per-axis
// tangents. A roll-unaware formula agrees on single axes and then drifts
// horizontally once roll is combined with pitch, which is exactly the failure
// AGENTS.md records from other mods in the catalogue.
void TestRollIsHandled() {
    std::printf("reticle offset: roll\n");
    const float clean[3] = {0.0f, 90.0f, 0.0f};

    float dx = 0.0f, dy = 0.0f;
    const float rollOnly[3] = {0.0f, 90.0f, 30.0f};
    Check(ComputeReticleOffset(clean, rollOnly, kFov, kW, kH, dx, dy), "roll-only projects");
    Check(NearEqual(dx, 0.0f, 0.01f) && NearEqual(dy, 0.0f, 0.01f),
          "pure roll leaves the reticle at centre - the aim point is on the view axis");

    // Pitch alone, then the same pitch with roll: the offset must rotate about
    // the centre, keeping its distance, not wander off at a new radius.
    float px = 0.0f, py = 0.0f;
    const float pitchOnly[3] = {-20.0f, 90.0f, 0.0f};
    ComputeReticleOffset(clean, pitchOnly, kFov, kW, kH, px, py);

    float rx = 0.0f, ry = 0.0f;
    const float pitchRoll[3] = {-20.0f, 90.0f, 30.0f};
    Check(ComputeReticleOffset(clean, pitchRoll, kFov, kW, kH, rx, ry), "pitch+roll projects");

    // Roll spins the view basis about the forward axis, so the aim direction's
    // angle off that axis is untouched and the offset only rotates. The pixel
    // radius is the right invariant to check despite the viewport not being
    // square: the vertical half-FOV is the horizontal one scaled by H/W, which
    // cancels the pixel scaling on that axis exactly.
    const float r0 = std::sqrt(px * px + py * py);
    const float r1 = std::sqrt(rx * rx + ry * ry);
    Check(NearEqual(r0, r1, 1.0f), "roll rotates the offset without changing its radius");
    Check(std::fabs(rx) > 1.0f, "roll with pitch does move the reticle horizontally");
}

void TestBehindCameraRejected() {
    std::printf("reticle offset: aim behind the view\n");
    const float clean[3]   = {0.0f, 90.0f, 0.0f};
    const float tracked[3] = {0.0f, -80.0f, 0.0f};  // head turned past 90 degrees
    float dx = 0.0f, dy = 0.0f;
    Check(!ComputeReticleOffset(clean, tracked, kFov, kW, kH, dx, dy),
          "aim point behind the tracked view is refused, not mirrored on screen");
}

void TestDegenerateViewport() {
    std::printf("reticle offset: degenerate input\n");
    const float a[3] = {0.0f, 90.0f, 0.0f};
    float dx = 0.0f, dy = 0.0f;
    Check(!ComputeReticleOffset(a, a, kFov, 0, kH, dx, dy), "zero width is refused");
    Check(!ComputeReticleOffset(a, a, 0.0f, kW, kH, dx, dy), "zero FOV is refused");
    Check(!ComputeReticleOffset(a, a, 180.0f, kW, kH, dx, dy), "180 degree FOV is refused");
}

}  // namespace

int RunReticleProjectionTests() {
    std::printf("\nReticle projection\n------------------\n");
    TestCentredWhenNotTracking();
    TestPureYaw();
    TestPurePitch();
    TestRollIsHandled();
    TestBehindCameraRejected();
    TestDegenerateViewport();
    return g_failures;
}
