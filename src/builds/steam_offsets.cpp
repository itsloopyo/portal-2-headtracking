// Steam Win32 build profiles for Portal 2's client.dll. Append-only: a new
// patch gets a new entry here and a new line at the top of kKnownProfiles in
// build_registry.cpp. Nothing in this file is ever edited in place.
//
// Rederive with .lab/ghidra_scripts/PortalCameraDiscovery.java, and read the
// running DLL's fingerprint with `pixi run check-fingerprint`.

#include "builds/build_registry.h"

namespace headtracking::builds {

// CViewSetup opens with the doubled screen-rect ints (x, x_old, y, y_old,
// width, width_old, height, height_old), which places the 48-byte
// m_matCustomViewMatrix at 0x24 and lands fov exactly on the confirmed 0x68.
// x and y are what split-screen tiles; width and height are the rendered
// viewport, which is the aspect ratio the FOV is widened for.
constexpr ViewSetupOffsets kViewSetupLayout_2025 = {
    0x70u,  // origin  (Vector x, y, z) - confirmed via runtime CViewSetup dump
    0x7Cu,  // angles  (QAngle pitch, yaw, roll)
    0x68u,  // fov
    0x6Cu,  // fovViewmodel - the float immediately after fov
    0x00u,  // rect x
    0x08u,  // rect y
    0x10u,  // rect width
    0x18u,  // rect height
};

// client.dll dated 2026-06-26 (Steam buildid 23934121). The RTTI vftable for
// CViewRender sits at rva 0x790CBC and slot 6 is RenderView, at the same rva
// 0x1F2620 the 2025-01-17 build used - this patch left the code layout alone
// (SizeOfImage is unchanged too; only TimeDateStamp and CheckSum moved). Same
// RVA, separate profile: the fingerprint is the routing key, and a shared
// entry could not describe both builds if the next patch does move it.

// The reticle element was identified by following the `crosshair` ConVar
// object (rva 0x9E4CE0 + 0x1C) to its two readers: one is CHudCrosshair's
// ShouldDraw, the other is this element's, at rva 0x28C130. Its Paint sits
// immediately after and draws four texture pieces - the two arcs and the dots -
// each positioned from width/2 and height/2. It is the portal gun's, and names
// C_WeaponPortalgun's type descriptor to prove it.
constexpr CrosshairOffsets kCrosshairLayout_20260626 = {
    0x28C200u,  // portal gun element's Paint - the arcs
    0x141EF0u,  // CHudCrosshair::Paint - the centre dots
    0x0D9B80u,  // screen width  - both Paints halve it for their centre
    0x0D9B60u,  // screen height
};

extern const BuildProfile kSteamProfile_20260626 = {
    "steam-win32-20260626",
    { 0x6A3E9243u, 0x00FF3000u, 0x00000000u },
    { 0x1F2620u, kViewSetupLayout_2025, kCrosshairLayout_20260626 },
};

// client.dll dated 2025-01-17. Offsets confirmed against the CViewSetup the
// running RenderView consumes.
//
// The crosshair addresses are left unset rather than copied from the
// 2026-06-26 profile: that build is not on hand to verify them against, and a
// wrong Paint RVA would detour an arbitrary function. Head tracking works as it
// always has here; only reticle compensation is unavailable, and it says so in
// the log.
extern const BuildProfile kSteamProfile_20250117 = {
    "steam-win32-20250117",
    { 0x678AE7C7u, 0x00FF3000u, 0x00AA88E7u },
    { 0x1F2620u, kViewSetupLayout_2025, {} },
};

}  // namespace headtracking::builds
