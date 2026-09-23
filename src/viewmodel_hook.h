// SPDX-License-Identifier: MIT
// Copyright (c) 2026 itsloopyo / CameraUnlock
#pragma once

namespace headtracking {

// Keeps the portal gun under the reticle while the head moves the view.
//
// The gun is drawn in its own pass at a much narrower FOV than the world, from
// the same view. Under head tracking that magnifies the angle between the gun
// (which sits on the clean aim axis) and the tracked view, so the gun swings
// about twice as far as the world does. This gives that one pass a view of its
// own - see viewmodel_view.h for the maths - drawn from the clean eye so a lean
// leaves the gun where it is.
//
// Installed only on a build profile carrying the viewmodel addresses; without
// them the gun is drawn as the game draws it.
class ViewmodelHook {
public:
    ViewmodelHook() = default;

    bool Install();
};

}  // namespace headtracking
