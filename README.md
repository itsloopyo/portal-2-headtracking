# Portal 2 Head Tracking

![Portal 2 running with this mod](https://raw.githubusercontent.com/itsloopyo/portal-2-headtracking/main/assets/readme-clip.gif)

An unofficial head tracking mod for Portal 2 that moves the view with your head while your mouse or controller keeps aiming, driven by OpenTrack over UDP, with no VR headset required.

## Features

- **Decoupled look and aim** - head tracking moves the camera; aim stays on your mouse or controller
- **6DOF positional tracking** - lean and peek with head position
- **Split-screen co-op tracking** - each player gets their own tracker feeding their own viewport

## Requirements

- [Portal 2](https://store.steampowered.com/app/620/Portal_2/) on Steam,
  legitimately purchased.
- A tracking source: [OpenTrack](https://github.com/opentrack/opentrack) on
  the PC, or a head-tracking phone app such as [Headcam](https://headcam.app)
  (free) that speaks the OpenTrack UDP protocol.
- Windows 10 or 11. Portal 2 is a 32-bit game, so the mod ships as a 32-bit
  DLL; 64-bit Windows runs it fine.

## Installation

1. Download the latest `Portal2HeadTracking-v<version>-installer.zip` from the
   [Releases page](https://github.com/itsloopyo/portal-2-headtracking/releases).
2. Extract it anywhere.
3. Double-click `install.cmd`. It finds your Steam copy of Portal 2 and
   deploys the mod plus the Ultimate ASI Loader.
4. Configure your tracker to send OpenTrack UDP to `127.0.0.1:4242` (see
   Setting Up OpenTrack below).
5. Launch Portal 2.

**If the installer cannot find your game**, point it at the folder yourself,
either with an environment variable:

```powershell
$env:PORTAL_2_PATH = "D:\Games\Portal 2"
.\install.cmd
```

or as the first argument:

```powershell
.\install.cmd "D:\Games\Portal 2"
```

### Manual Installation

The `-nexus` ZIP holds the mod at `bin\Portal2HeadTracking.asi`, plus
`LICENSE`, `THIRD-PARTY-NOTICES.md` and this README at its root. It assumes
you already have an ASI loader, and Portal 2 needs it in a specific place:

1. Extract the ZIP over your Portal 2 folder, so the `.asi` lands in
   `Portal 2\bin\`.
2. Put an ASI loader in that same `bin\` folder, named `winmm.dll`. The
   installer ZIP carries one at `vendor\ultimate-asi-loader\dinput8.dll`;
   copy it to `Portal 2\bin\winmm.dll`.

Both files must be in `bin\`, not next to `portal2.exe`. Source loads
`tier0.dll` from `bin\` with an altered search path, so a proxy DLL at the
game root is never loaded at all.

## Setting Up OpenTrack

The mod listens for OpenTrack pose data on UDP port `4242`, on every network
interface. One datagram is six little-endian 64-bit floats in the order
`x, y, z, yaw, pitch, roll`: position in centimetres, rotation in degrees, 48
bytes in total. Anything that sends that to that port drives the view.
OpenTrack's **UDP over network** output sends exactly this, and the steps below
set it up.

1. Install [OpenTrack](https://github.com/opentrack/opentrack/releases).
2. Pick a tracker under **Input**, using the notes below.
3. Set **Output** to **UDP over network**, host `127.0.0.1`, port `4242`.
4. Press **Start**. Tracking and the game can start in either order.

### Webcam

OpenTrack ships a `neuralnet tracker` input that reads a plain webcam. Select it
under **Input**, pick your camera in its settings, and use the output settings
above. How well it tracks depends on your camera and your lighting, so try it
before buying anything.

### Phone

A phone app can reach the mod directly, with no OpenTrack on the PC, if it sends
the datagram described above. Point it at this PC's IP address (run `ipconfig`
to find it) on port `4242`. Not every phone tracker speaks this protocol, so
check yours for an OpenTrack or UDP output option first. [Headcam](https://headcam.app)
sends it, and I wrote it so decent tracking is free for anyone who already owns
a phone.

Sending direct works when the app filters its own signal on the device. The
mod's smoothing is sized to take the edge off a clean signal rather than to
rescue a noisy one, so a raw feed sent direct will jitter. If it does, point the
app at OpenTrack's **UDP over network** *input* on some other port, say 5252,
and let OpenTrack's filters and curves clean it up before its output forwards to
`127.0.0.1:4242`.

Anything arriving from outside `127.0.0.0/8` counts as a remote connection and
is smoothed with `RemoteSmoothing` rather than `LocalSmoothing`. That includes a
tracker on this very PC that sends to the machine's own LAN address, because the
mod reads the source address and not the machine.

### Headset or other hardware

If your device has an OpenTrack input driver, select it under **Input** and use
the same output settings. OpenTrack's own **Input** list is the authority on
what it can read; the mod only ever sees what OpenTrack sends.

### Centring

Centring belongs to your tracker. The mod subtracts no centre of its own: it
applies the pose it receives exactly as it arrives, so a stream of zeros holds
the view where the game itself puts it. Press the centre control in your tracker
(OpenTrack's **Center** bind, or the CENTER button in Headcam) and the tracker
zeroes its own output, which leaves the view centred with the mod doing nothing.

That is why there is no centre hotkey here and nothing to re-centre in game. Two
centres in series would drift apart, because each side re-centres at moments the
other cannot see, and you would end up pressing twice to centre once. If the
view sits off to one side, centre it in the tracker.

## Controls

Two equivalent binding sets, use whichever your keyboard has:

| Action              | Nav-cluster | Chord           |
|---------------------|-------------|-----------------|
| Toggle tracking     | `End`       | `Ctrl+Shift+Y`  |
| Cycle tracking mode | `Page Up`   | `Ctrl+Shift+G`  |
| Toggle yaw mode     | `Page Down` | `Ctrl+Shift+H`  |

Both sets fire the same action, so a keyboard without a nav cluster loses
nothing. The nav-cluster key is ignored while Ctrl+Shift is held, so a chord
press never triggers two actions.

`Page Up` / `Ctrl+Shift+G` cycles tracking mode:

1. Normal head-tracked gameplay (rotation and position)
2. Rotation only, position disabled
3. Position only, rotation disabled
4. Back to normal

`Page Down` / `Ctrl+Shift+H` switches yaw mode between horizon-locked
(default) and camera-local. See `WorldSpaceYaw` below.

## Configuration

`HeadTracking.ini` is written next to `portal2.exe` on first launch. Edit it
and relaunch the game.

```ini
; Portal 2 head tracking - default config

[Network]
Port=4242
; Split-screen co-op: player 2's tracker sends to Port2
Port2=4243
SplitScreenPlayer2=1
EnableOnStartup=1

[Sensitivity]
Yaw=1
Pitch=1
Roll=1
InvertYaw=0
InvertPitch=0
InvertRoll=0

[Smoothing]
; Picked per connection from the tracker's source address, and applied
; to both rotation and position. 0 = no smoothing, 1 = heavy.
; LocalSmoothing: tracker runs on this machine (loopback)
LocalSmoothing=0
; RemoteSmoothing: tracker is a remote device on the network
RemoteSmoothing=0.15

[Deadzone]
; Degrees of head movement ignored around centre, per axis
Yaw=0
Pitch=0
Roll=0

[Position]
; 6DOF head position, applied to the render view origin only
Enabled=1
; Source units per metre of head movement (1 unit = 1 inch; 39.37 = 1:1).
; The main lean-strength knob.
WorldScale=39.37
SensX=1
SensY=1
SensZ=1
; Flip an axis if leaning moves the view the wrong way. Inversion is applied
; after the limits below, so flipping Z keeps the generous forward allowance
; on leaning in.
InvertX=0
InvertY=0
InvertZ=0
; Movement envelope in metres before world scaling. Z is asymmetric: LimitZ
; is the forward lean, LimitZBack the backward one.
LimitX=0.3
LimitY=0.2
LimitZ=0.4
LimitZBack=0.1

[Hotkeys]
; Virtual-key codes: End, Page Down, Page Up
Toggle=0x23
YawMode=0x22
ModeCycle=0x21

[View]
; 1 = horizon-locked yaw (default), 0 = camera-local yaw
WorldSpaceYaw=1
; Field of view, same units as the game's fov_desired cvar (30-150).
; 0 = leave the game's FOV alone.
Fov=0
; The portal gun's own FOV. The game draws it at the equivalent of 50, so
; LOWER this to shrink the gun. 0 = leave it alone.
FovViewmodel=0

[Debug]
; 1 logs the build profile, tracker connection and applied head pose to
; Portal2HeadTracking.log next to portal2.exe, fresh every launch. That is
; the file to attach to a bug report - leave it on.
LogToFile=1
```

Smoothing is picked from the packet's source address, not from which machine
the tracker runs on. A tracker running on this PC but sending to your LAN
address instead of `127.0.0.1` therefore counts as remote and gets
`RemoteSmoothing`; point it at `127.0.0.1` if you want `LocalSmoothing`.

### Field of View

Portal 2's own `fov_desired` only goes from 75 to 90, and it is a userinfo
cvar, so whatever you set is also sent to the server. `Fov` is written
straight into the view the frame is rendered from instead: no ceiling, no
`sv_cheats`, and the server still sees your real `fov_desired`.

The number means what it means in the console, a horizontal FOV for a 4:3
screen, which the mod widens for your actual aspect ratio exactly as the
engine does. So `Fov=90` matches the game's maximum setting, and on a 16:9
monitor both render at 106.3 degrees horizontally.

`FovViewmodel` is separate because the portal gun is drawn through its own
projection that Portal 2 gives you no way to change. Widening `Fov` alone
leaves the gun looking oversized against the wider world; lower
`FovViewmodel` to shrink it. The game draws it at the equivalent of 50, so
try 40 to 45 alongside a wide `Fov`.

Both are part of the tracked view, so toggling tracking off with `End`
returns you to the game's own FOV, and turning it back on restores yours.

## Troubleshooting

**Mod not loading.** Read `Portal2HeadTracking.log` next to `portal2.exe`. It
is written fresh every launch, so it only ever holds the session you just
played; the launch before it is kept as `Portal2HeadTracking.prev.log`, which
is the one to send if the game crashed and you relaunched.

- No log file at all means the ASI loader is not injecting. Check that
  `winmm.dll` and `Portal2HeadTracking.asi` are both in `Portal 2\bin\`, not
  next to `portal2.exe`.
- `no build profile matches this client.dll - staying dormant` means Portal 2
  was updated and this release does not know the new build yet. The mod
  deliberately does nothing rather than hook a stale address, so the game is
  unaffected. Check the releases page.

**No tracking response.** The mod loads but the view does not move.

- No `tracking source connected` line in the log means nothing is arriving on
  UDP 4242. Check the tracker app's target address and port, and your
  firewall.
- `UDP port 4242 busy` means something else has the port, usually another
  game you left running. Close it and tracking starts within about half a
  second; the mod keeps retrying the bind and logs `tracking is live` when it
  gets in, so there is no need to restart Portal 2.
- Check tracking is not toggled off: press `End` (or `Ctrl+Shift+Y`).

**Jittery or unstable tracking.** Raise the smoothing value that applies to
your setup: `RemoteSmoothing` for a phone or other network tracker,
`LocalSmoothing` for a tracker on this PC. Try 0.3 and work down. A webcam
tracker also jitters in poor lighting, so light your face evenly first.

**Wrong rotation or lean axis.**

- Leaning moves the view the wrong way: flip `InvertX`, `InvertY` or
  `InvertZ` under `[Position]`. Trackers disagree on whether they report in
  your frame or the camera's mirrored view of it, so one of the two is right
  for your setup and it is obvious within a second of leaning.
- Head rotation is inverted: flip `InvertYaw`, `InvertPitch` or `InvertRoll`
  under `[Sensitivity]`.
- Yaw feels wrong when looking steeply up or down: toggle between
  horizon-locked and camera-local yaw with `Page Down` (or `Ctrl+Shift+H`).

**The crosshair does not sit where portals land** when your head is turned.
Known limitation of this early build, see the note at the top. The portal
itself still goes where the mouse points.

## Updating

Download the new release and run `install.cmd` again. Your `HeadTracking.ini`
is preserved.

## Uninstalling

Run `uninstall.cmd`. This removes the mod files. The ASI loader is only
removed if the installer put it there; use `uninstall.cmd /force` to remove it
anyway (do that only if no other mod needs it).

`uninstall.cmd` also deletes `HeadTracking.ini` and the log, so your tuning
goes with it. Copy the file first if you want to keep it. Uninstalling through
the Lopari launcher leaves it in place.

## Building from Source

Requires Visual Studio 2022 with the C++ workload, CMake, and pixi. Portal 2
is a 32-bit Source Engine game, so the DLL is built for `x86`.

```powershell
git clone --recursive https://github.com/itsloopyo/portal-2-headtracking
cd portal-2-headtracking
pixi run build
pixi run package
```

## Community & Support

- Discord: [Loop's Head Tracking Hangout](https://discord.com/invite/dxyZdyFNT9) - setup help, bug reports, and new-release announcements
- [Lopari](https://lopari.app) - free Windows launcher with one-click install and launch for the released head-tracking mods
- [Headcam](https://headcam.app) - free app that turns your iPhone or Android phone into the head tracker

## License

MIT License - see [LICENSE](LICENSE) for details.

Third-party components bundled in or linked into the release are listed in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) with their own licenses.

## Credits

- Portal 2 and the Source Engine (C) Valve Corporation.
- [OpenTrack](https://github.com/opentrack/opentrack) (ISC) for the tracking protocol.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) by ThirteenAG (MIT).
- [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu (BSD-2-Clause).
- [CameraUnlock core](https://github.com/itsloopyo/cameraunlock-core) (MIT) for the shared tracking pipeline.

## Disclaimer

This mod is not affiliated with, endorsed by, or supported by Valve
Corporation. Neither download contains any Valve code or assets, and the mod
requires a legitimately purchased copy of Portal 2. The demo clip at the top of
this page is recorded gameplay and remains Valve's - see
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md). Use at your own risk.
