#include "plugin.h"

#include "camera_hook.h"
#include "crosshair_hook.h"
#include "debug_log.h"
#include "hotkey_handler.h"

namespace headtracking {

// Deliberately leaked, never destroyed. A function-local static would register
// ~Plugin with atexit, and MSVC runs those from LdrShutdownProcess AFTER
// DllMain(DLL_PROCESS_DETACH) - so the careful "don't tear down on process
// exit" guard in dllmain.cpp would be undone by the destructor running the
// teardown anyway, under the loader lock, joining threads the OS has already
// killed. The process is going away; let it.
Plugin& GetPlugin() {
    static Plugin* instance = new Plugin();
    return *instance;
}

Plugin::Plugin() = default;
Plugin::~Plugin() = default;

void Plugin::Initialize() {
    m_config = Config::LoadOrCreateDefault();
    SetFileLogging(m_config.log_to_file);
    m_enabled.store(m_config.enabled_on_startup);
    m_worldSpaceYaw.store(m_config.world_space_yaw);

    m_activePlayers = m_config.splitscreen_player2 ? kMaxPlayers : 1;
    m_feeds[0].Start(m_config.port, m_config);
    if (m_activePlayers > 1) {
        m_feeds[1].Start(m_config.port2, m_config);
    }

    m_cameraHook = std::make_unique<CameraHook>();
    if (!m_cameraHook->Install()) {
        HT_LOG("[plugin] camera hook not installed - mod is dormant (view unmodified)");
        // Not a fatal error: a dormant hook (unrecognised game build) must
        // leave the game fully playable. Hotkeys/receiver still run so a log
        // inspection shows tracking data arriving.
    } else {
        // Only meaningful once the camera hook has resolved the build, and only
        // worth installing if the view is actually being modified.
        m_crosshairHook = std::make_unique<CrosshairHook>();
        m_crosshairHook->Install();
    }

    m_hotkeys = std::make_unique<HotkeyHandler>();
    m_hotkeys->Start(*this, m_config.recenter_vk, m_config.toggle_vk, m_config.yaw_mode_vk,
                     m_config.mode_cycle_vk);
    // Trackers disagree on whether they report in the user's frame (+X right,
    // +Z forward) or the camera's mirrored view of it. Logging the effective
    // inversion means a "leaning moves the wrong way" report arrives with the
    // answer already in it.
    HT_LOG("[plugin] position inversion: X=%d Y=%d Z=%d (flip in HeadTracking.ini "
           "[Position] if leaning moves the view the wrong way)",
           m_config.pos_invert_x ? 1 : 0, m_config.pos_invert_y ? 1 : 0,
           m_config.pos_invert_z ? 1 : 0);
    HT_LOG("[plugin] initialized");
}

void Plugin::ToggleEnabled() {
    const bool next = !m_enabled.load();
    m_enabled.store(next);
    for (int i = 0; i < m_activePlayers; ++i) m_feeds[i].SetEnabled(next);
}

void Plugin::Recenter() {
    for (int i = 0; i < m_activePlayers; ++i) m_feeds[i].Recenter();
}

void Plugin::ToggleYawMode() {
    const bool next = !m_worldSpaceYaw.load();
    m_worldSpaceYaw.store(next);
    HT_LOG("[plugin] yaw mode -> %s", next ? "world-space" : "camera-local");
}

void Plugin::CycleTrackingMode() {
    for (int i = 0; i < m_activePlayers; ++i) m_feeds[i].CycleMode();
    HT_LOG("[plugin] tracking mode -> %s", TrackingModeName());
}

const char* Plugin::TrackingModeName() const { return m_feeds[0].ModeName(); }

void Plugin::Update(int slot) {
    if (!HasFeed(slot)) return;
    m_feeds[slot].Update();
}

bool Plugin::GetRotationRadians(int slot, float& yaw, float& pitch, float& roll) const {
    if (!HasFeed(slot)) return false;
    return m_feeds[slot].GetRotationRadians(yaw, pitch, roll);
}

bool Plugin::GetPositionOffset(int slot, float& x, float& y, float& z) const {
    if (!HasFeed(slot)) return false;
    return m_feeds[slot].GetPositionOffset(x, y, z);
}

}  // namespace headtracking
