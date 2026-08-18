#include "pch.h"

#include "plugin.h"
#include "debug_log.h"
#include "version.h"

namespace {

DWORD WINAPI BootstrapThread(LPVOID) {
    headtracking::OpenLogFile();
    HT_LOG("[main] Portal2HeadTracking %s loaded into pid %lu",
           HEADTRACKING_VERSION_STRING, GetCurrentProcessId());

    headtracking::GetPlugin().Initialize();
    return 0;
}

// Takes a permanent reference on this module, so an unload cannot pull the code
// out from under the bootstrap thread or the installed hooks.
void PinSelf() {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                       reinterpret_cast<LPCWSTR>(&BootstrapThread), &self);
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            // Pin the module before starting the thread that outlives DllMain.
            // Something in Portal 2's startup FreeLibrary's us - the detach
            // arrives with reserved == nullptr (an explicit unload, not process
            // exit) while the bootstrap thread is still in Config::Load, and the
            // thread then runs on in freed memory. That is a crash in
            // "Portal2HeadTracking.asi_unloaded", usually
            // STATUS_INVALID_EXCEPTION_HANDLER, and it takes the game with it.
            //
            // Pinning makes FreeLibrary a no-op for us, which is the right
            // lifetime anyway: the bootstrap thread runs for the whole session,
            // and the render hook is never uninstalled (see camera_hook.h) - so
            // this code must stay mapped for as long as the process lives.
            PinSelf();
            CreateThread(nullptr, 0, BootstrapThread, nullptr, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            // Nothing. Not on process exit (reserved != nullptr): the OS has
            // already killed our worker threads, possibly mid-syscall or
            // holding the log mutex, so joining or unhooking then can hang the
            // game on the way out. And not on a FreeLibrary unload either:
            // DllMain runs under the loader lock, and Shutdown() joins the
            // hotkey and receiver threads whose own exit path (LdrShutdownThread
            // running every other DLL's DLL_THREAD_DETACH) needs that same
            // lock - a textbook deadlock. An ASI plugin is never unloaded in
            // practice; the process teardown reclaims everything.
            break;
    }
    return TRUE;
}
