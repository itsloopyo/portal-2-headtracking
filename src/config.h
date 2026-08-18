#pragma once

#include <cstdint>
#include <string>

#include "hotkeys.h"

namespace headtracking {

// The one home for every shipped default. The struct initializers below, the
// generated HeadTracking.ini and the fallback used when a key is missing or
// malformed all read from here, so a default cannot drift between the file a
// user is handed and the value the mod actually runs with.
constexpr uint16_t kDefaultPort  = 4242;
constexpr uint16_t kDefaultPort2 = 4243;
constexpr bool     kDefaultSplitScreenPlayer2 = true;
constexpr bool     kDefaultEnableOnStartup    = true;

constexpr float kDefaultSensitivity = 1.0f;
constexpr float kDefaultDeadzone    = 0.0f;

constexpr float kDefaultLocalSmoothing  = 0.0f;
constexpr float kDefaultRemoteSmoothing = 0.15f;

constexpr bool  kDefaultPosEnabled     = true;
constexpr float kDefaultPosSensitivity = 1.0f;
constexpr float kDefaultPosLimitX      = 0.30f;
constexpr float kDefaultPosLimitY      = 0.20f;
constexpr float kDefaultPosLimitZ      = 0.40f;
constexpr float kDefaultPosLimitZBack  = 0.10f;
constexpr float kDefaultPosWorldScale  = 39.37f;

constexpr bool  kDefaultWorldSpaceYaw = true;
constexpr float kDefaultFovOverride   = 0.0f;
constexpr bool  kDefaultLogToFile     = false;

// An override outside this band is refused: the renderer builds a projection
// from tan(fov/2), so a value at or past 180 has none at all and one near it
// stretches the frame into uselessness.
constexpr float kMinFovOverride = 30.0f;
constexpr float kMaxFovOverride = 150.0f;

struct Config {
    uint16_t port = kDefaultPort;
    // Split-screen co-op renders one viewport per player, each fed by its own
    // tracker: player 1 on `port`, player 2 on `port2`.
    uint16_t port2 = kDefaultPort2;
    bool splitscreen_player2 = kDefaultSplitScreenPlayer2;
    bool enabled_on_startup = kDefaultEnableOnStartup;

    float sens_yaw = kDefaultSensitivity;
    float sens_pitch = kDefaultSensitivity;
    float sens_roll = kDefaultSensitivity;
    bool invert_yaw = false;
    bool invert_pitch = false;
    bool invert_roll = false;

    // Smoothing is chosen per connection: local for a tracker on this machine
    // (loopback), remote for a device on the network. Both cover rotation and
    // position.
    float local_smoothing = kDefaultLocalSmoothing;
    float remote_smoothing = kDefaultRemoteSmoothing;

    float deadzone_yaw = kDefaultDeadzone;
    float deadzone_pitch = kDefaultDeadzone;
    float deadzone_roll = kDefaultDeadzone;

    // Positional (6DOF) tracking. Head displacement is applied to the render
    // view origin only - the game's clean camera origin is untouched, so
    // portal aim / raycasts / physics are unaffected. See camera_hook.cpp.
    bool  pos_enabled    = kDefaultPosEnabled;
    float pos_sens_x     = kDefaultPosSensitivity;
    float pos_sens_y     = kDefaultPosSensitivity;
    float pos_sens_z     = kDefaultPosSensitivity;
    bool  pos_invert_x   = false;
    bool  pos_invert_y   = false;
    bool  pos_invert_z   = false;
    // Head-movement envelope in metres (clamped before world scaling). Z is
    // asymmetric: pos_limit_z = forward lean (generous), z_back = backward.
    float pos_limit_x      = kDefaultPosLimitX;
    float pos_limit_y      = kDefaultPosLimitY;
    float pos_limit_z      = kDefaultPosLimitZ;
    float pos_limit_z_back = kDefaultPosLimitZBack;
    // Source world units per metre of head movement. 1 unit = 1 inch, so
    // 39.37 is 1:1 with real-world head movement. Primary lean tuning knob.
    float pos_world_scale  = kDefaultPosWorldScale;

    int recenter_vk   = hotkeys::kVkHome;
    int toggle_vk     = hotkeys::kVkEnd;
    int yaw_mode_vk   = hotkeys::kVkPageDown;
    // Page Up: cycles 6DOF -> rotation -> position.
    int mode_cycle_vk = hotkeys::kVkPageUp;

    // true  = horizon-locked yaw (yaw around world up axis, default)
    // false = camera-local yaw (yaw composed with pitch/roll)
    bool world_space_yaw = kDefaultWorldSpaceYaw;

    // Field of view in the same units as the game's own `fov_desired` cvar:
    // horizontal degrees referenced to a 4:3 screen, which the mod widens for
    // the actual viewport exactly as the engine does. 0 = leave the game's FOV
    // alone. Portal 2 clamps fov_desired to 75-90; this does not,
    // because it is written straight into the render view the frame is built
    // from - which is also what keeps the crosshair reprojection consistent
    // with it.
    float fov_override = kDefaultFovOverride;

    // The portal gun is drawn through a second FOV (CViewSetup::fovViewmodel),
    // which Portal 2 gives the player no way to change at all - the
    // `viewmodel_fov` cvar is cheat-flagged AND does not reach this field. A
    // wider world FOV leaves the gun looking oversized against it; LOWER this
    // to shrink the gun. The game draws it at the equivalent of 50 in these
    // units, so 40-45 shrinks it noticeably. Same units, same 0 = leave alone.
    float fov_viewmodel_override = kDefaultFovOverride;

    bool log_to_file = kDefaultLogToFile;

    static std::string IniPath();  // <game folder>\HeadTracking.ini
    static Config LoadOrCreateDefault();
    static void WriteDefault(const std::string& path);
};

}  // namespace headtracking
