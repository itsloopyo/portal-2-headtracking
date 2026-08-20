#pragma once

#include <atomic>
#include <memory>

#include "config.h"
#include "tracker_feed.h"

namespace headtracking {

class CameraHook;
class CrosshairHook;
class HotkeyHandler;

// Mod-level coordinator: owns the config, the per-player tracker feeds, the
// render-view hook and the hotkeys, and is the single object the detour asks
// for a viewport's pose. Constructed once and never destroyed (see GetPlugin).
class Plugin {
public:
    // Player 1 listens on Config::port, player 2 on Config::port2. Slot comes
    // from the split-screen viewport rect (see camera_hook.cpp).
    static constexpr int kMaxPlayers = 2;

    Plugin();
    ~Plugin();

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    void Initialize();

    bool IsEnabled() const { return m_enabled.load(); }
    void ToggleEnabled();

    bool IsWorldSpaceYaw() const { return m_worldSpaceYaw.load(); }
    void ToggleYawMode();

    void CycleTrackingMode();
    const char* TrackingModeName() const;

    // How many tracker feeds are live (1, or 2 with split-screen player 2).
    // The camera hook needs this to decide whether an offset viewport is a
    // second player's tile or just an unexpected rect.
    int ActivePlayers() const { return m_activePlayers; }

    // Per-viewport tracking state. `slot` is clamped to the configured player
    // count; a slot with no tracker feed simply reports no data.
    void Update(int slot);
    bool GetRotationRadians(int slot, float& yaw, float& pitch, float& roll) const;
    bool GetPositionOffset(int slot, float& x, float& y, float& z) const;

    const Config& GetConfig() const { return m_config; }

private:
    bool HasFeed(int slot) const { return slot >= 0 && slot < m_activePlayers; }

    Config m_config;
    std::atomic<bool> m_enabled{false};
    std::atomic<bool> m_worldSpaceYaw{kDefaultWorldSpaceYaw};

    TrackerFeed m_feeds[kMaxPlayers] = { TrackerFeed(0), TrackerFeed(1) };
    int m_activePlayers = 1;

    std::unique_ptr<CameraHook>    m_cameraHook;
    std::unique_ptr<CrosshairHook> m_crosshairHook;
    std::unique_ptr<HotkeyHandler> m_hotkeys;
};

Plugin& GetPlugin();

}  // namespace headtracking
