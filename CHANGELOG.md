# Changelog

## [0.2.0] - 2026-08-20

### Changed

- Maintenance release (no user-facing changes).

## [0.1.0] - 2026-08-20

### Added

- remove the in-game recentre control, default logging on

### Other

- Hello world

All notable changes to Portal2HeadTracking will be documented in this file.

## [Unreleased]

### Added
- Initial working build: render-only head-tracking view hook on
  `CViewRender::RenderView` (Source Engine `client.dll`), gated by a
  PE-fingerprint build-profile registry. Look (head) and aim (mouse) are fully
  decoupled, so portal placement, traces, and physics read the clean camera.
- Added 3DOF rotation and 6DOF position tracking through the shared
  cameraunlock-core pipeline (OpenTrack UDP receiver, interpolator, processor,
  session).
- Added split-screen co-op support: each player's viewport is fed by its own
  tracker (player 1 on UDP 4242, player 2 on 4243, both configurable). The slot
  comes from the split-screen viewport rect.
- Added horizon-locked (world-space) yaw by default and a camera-local yaw mode
  on Page Down, composing the head rotation about the camera's own axes.
- Added configurable sensitivity, smoothing, deadzone, and inversion per axis,
  plus tracking toggle and 6DOF/rotation/position mode cycling on the
  nav-cluster keys with Ctrl+Shift chord alternatives.
- Added build profiles for Steam `client.dll` 2025-01-17 and 2026-06-26
  (buildid 23934121). Profiles are append-only, so one release supports every
  build it has ever known; an unrecognised build leaves the mod fully dormant
  and the game vanilla.
- Added `pixi run check-fingerprint`, which prints the installed `client.dll`
  fingerprint as a paste-ready build-profile stub, and a daily patch-watch
  workflow that opens a rederive issue when Steam publishes a new Portal 2
  build.
- Added Ultimate ASI Loader injection via `winmm.dll` deployed to
  `<game>\bin`. install.cmd and uninstall.cmd follow the unified launcher
  contract, and the launcher manifest uses native (`delivery_mode: manifest`)
  deployment.
- Added `[View] Fov`, which sets the field of view in the same units as the
  game's `fov_desired` (a 4:3-referenced horizontal FOV, widened for your real
  aspect ratio as the engine does it). Portal 2's own cvar only spans 75-90 and
  is sent to the server as userinfo; this is written into the render view
  instead, so it has no ceiling, needs no `sv_cheats`, and leaves the server's
  view of `fov_desired` alone. `0` (the default) leaves the game's FOV alone.
- Added `[View] FovViewmodel`, which does the same for the FOV the portal gun
  is drawn with, something Portal 2 exposes no way to change (`viewmodel_fov`
  is cheat-flagged and does not reach the field). Lower it to shrink the gun
  against a wider world FOV.
- Extended the `[view]` diagnostic line to report the viewport size and both
  FOVs, so a bug report carries the aspect ratio the view was rendered at.

### Changed
- Removed the in-game recentre control. The tracker app owns the centre now:
  centre with your tracker's own control (OpenTrack's Center bind, the CENTER
  button in a phone app, SteamVR's reset) and the mod applies the pose it
  receives as absolute. Keeping a second centre inside the mod meant the two
  could drift apart, and switching trackers meant recentring on both sides. The
  `Home` / `Ctrl+Shift+T` hotkey and the `[Hotkeys] Recenter` INI key are gone.
  In split-screen each player already centred from their own tracker app, so
  that path is unchanged.
- `[Debug] LogToFile` now defaults to on. Off, `Portal2HeadTracking.log` held a
  single loader line, so every "no head tracking" report cost a round trip
  asking the user to enable logging and play again. `LogToFile=0` is now read
  before the log is opened, so it creates neither `Portal2HeadTracking.log` nor
  `Portal2HeadTracking.prev.log` rather than leaving a truncated pair behind.
- The log now keeps one previous generation as `Portal2HeadTracking.prev.log`.
  It is still truncated per launch, so a crash no longer erases the session
  worth reading when the user relaunches.
- Split smoothing into two keys, `[Smoothing] LocalSmoothing` (default 0.0) and
  `[Smoothing] RemoteSmoothing` (default 0.15), selected per connection from
  the tracker's source address. Both cover rotation and position.

### Fixed
- Fixed `[Position] LimitY` bounding upward head travel only. The downward
  bound is a separate field in the tracking pipeline and was left at its own
  0.20m default, so `LimitY = 0.40` gave 0.40m up and 0.20m down. The one INI
  key now sets both, which is what the default (0.20) always looked like it was
  doing.
- Fixed an invalid `[Network] Port` / `Port2` falling back to the default
  silently. It now logs which port the mod actually bound - the symptom is "no
  head tracking at all", and nothing else in the log explained it.

### Removed
- Removed the `[Smoothing] Amount` and `[Position] Smoothing` keys, replaced by
  the two keys above.
- Removed the hidden 0.15 baseline smoothing floor, so a tracker on this
  machine now gets zero-latency tracking by default.
