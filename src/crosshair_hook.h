#pragma once

namespace headtracking {

// Puts the crosshair back on the point the player is actually aiming at.
//
// Portal shots already fly along the clean mouse aim - the render-view hook
// never touches the game's own camera - but the crosshair is drawn at screen
// centre, which after a head turn is no longer where a portal will land. This
// closes that gap.
//
// It does so with Valve's own projection rather than any screen maths of ours.
// Both crosshair elements compute their position as
//
//     AngleVectors(CurrentViewAngles() + m_vecCrosshairOffsetAngle, forward)
//     ScreenTransform(CurrentViewOrigin() + forward, screen)
//
// and CurrentViewAngles() is the head-tracked angles, because RenderView seeds
// those globals from the CViewSetup we mutate. So writing (clean - tracked)
// into m_vecCrosshairOffsetAngle makes that sum the clean aim angles exactly,
// and the game projects them through the very matrices the frame was built
// from. It follows the [View] Fov override for free, and it cannot drift out of
// step with the camera the way a re-derived projection would.
//
// Installed only on a build profile carrying the crosshair addresses; without
// them the game keeps its vanilla centred crosshair.
class CrosshairHook {
public:
    CrosshairHook() = default;

    bool Install();
};

}  // namespace headtracking
