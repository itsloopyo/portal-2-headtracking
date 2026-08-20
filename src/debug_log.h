#pragma once

#include <Windows.h>
#include <string>

#include "cameraunlock/logging/file_log.h"

namespace headtracking {

// Opens Portal2HeadTracking.log next to the game EXE (truncated each launch),
// keeping the previous launch as Portal2HeadTracking.prev.log. Called once from
// the bootstrap thread before the first HT_LOG so the loader-presence line is
// captured, and only when [Debug] LogToFile is on - the caller checks first, so
// LogToFile=0 leaves no file of either generation behind.
inline void OpenLogFile() {
    wchar_t buf[MAX_PATH] = {};
    const DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring dir;
    if (len > 0 && len < MAX_PATH) {
        std::wstring exe(buf, len);
        const auto slash = exe.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dir = exe.substr(0, slash + 1);
    }
    const std::wstring path = dir + L"Portal2HeadTracking.log";
    // Keep one previous generation. The render hook can take the game down with
    // it, and the user relaunches before sending the log - a plain truncate
    // would erase the session actually worth reading.
    const BOOL  rotated   = MoveFileExW(path.c_str(),
                                        (dir + L"Portal2HeadTracking.prev.log").c_str(),
                                        MOVEFILE_REPLACE_EXISTING);
    const DWORD rotateErr = rotated ? 0u : GetLastError();
    cameraunlock::logging::Open(path);

    // The open below truncates regardless, so a failed rotation means the
    // previous session is gone and .prev.log holds something older than the
    // README promises. ERROR_FILE_NOT_FOUND is a first launch with nothing to
    // rotate.
    if (!rotated && rotateErr != ERROR_FILE_NOT_FOUND) {
        cameraunlock::logging::Line(
            "[main] could not rotate the previous log to Portal2HeadTracking.prev.log "
            "(error %lu); that file holds an older session, not the previous launch",
            rotateErr);
    }
}

}  // namespace headtracking

#define HT_LOG(...) ::cameraunlock::logging::Line(__VA_ARGS__)
