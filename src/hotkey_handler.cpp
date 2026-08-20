#include "hotkey_handler.h"

#include "cameraunlock/input/chord_hotkeys.h"
#include "debug_log.h"
#include "hotkeys.h"
#include "plugin.h"

namespace headtracking {

void HotkeyHandler::Start(Plugin& plugin, int toggle_vk, int yaw_mode_vk,
                          int mode_cycle_vk) {
    using cameraunlock::input::ChordGuarded;
    using cameraunlock::input::NavGuarded;

    const auto toggle = [&plugin]() {
        plugin.ToggleEnabled();
        HT_LOG("[hotkey] toggle -> %s", plugin.IsEnabled() ? "on" : "off");
    };
    const auto yawMode = [&plugin]() {
        plugin.ToggleYawMode();
        HT_LOG("[hotkey] yaw mode -> %s",
               plugin.IsWorldSpaceYaw() ? "world-space" : "camera-local");
    };
    const auto modeCycle = [&plugin]() {
        plugin.CycleTrackingMode();
        HT_LOG("[hotkey] mode cycle -> %s", plugin.TrackingModeName());
    };

    // Nav-cluster bindings are suppressed while Ctrl+Shift is held so the chord
    // path is the sole trigger and a remapped nav key cannot fire twice.
    m_poller.SetToggleKey(toggle_vk, NavGuarded(toggle));
    m_poller.AddHotkey(yaw_mode_vk, NavGuarded(yawMode));
    m_poller.AddHotkey(mode_cycle_vk, NavGuarded(modeCycle));

    m_poller.AddHotkey(hotkeys::kVkY, ChordGuarded(toggle));
    m_poller.AddHotkey(hotkeys::kVkH, ChordGuarded(yawMode));
    m_poller.AddHotkey(hotkeys::kVkG, ChordGuarded(modeCycle));

    m_poller.Start(kPollIntervalMs);
}

}  // namespace headtracking
