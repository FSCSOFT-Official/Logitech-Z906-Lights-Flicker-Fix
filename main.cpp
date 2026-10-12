#include <windows.h>
#include <shellapi.h>
#include "resource.h"
#include "Config.h"
#include "AudioPlayer.h"
#include "AudioDetector.h"
#include "TrayManager.h"

#define WM_TRAY_CALLBACK (WM_USER + 10)
#define TIMER_TICK_ID 1001

static const wchar_t* WINDOW_CLASS_NAME = L"LogitechKeepAliveHiddenWnd";

static AppConfig g_config;
static AudioPlayer g_audioPlayer;
static AudioDetector g_audioDetector;
static TrayManager g_trayManager;
static TrayState g_trayState = TrayState::Idle;
static int g_remainingSeconds = 0;
static UINT g_uTaskbarRestartMsg = 0;

void SwitchToMode(HWND /*hWnd*/, AppMode newMode) {
    if (g_config.mode == newMode) return;

    g_config.mode = newMode;
    g_config.Save();

    // Stop existing playback when switching modes
    g_audioPlayer.Stop();
    g_remainingSeconds = 0;
    g_trayState = TrayState::Idle;
    g_trayManager.UpdateState(g_trayState, g_config.mode);
}

void StartManualPlayback(HWND /*hWnd*/, int durationSeconds) {
    g_config.mode = AppMode::Manual;
    if (g_audioPlayer.Start(g_config.frequencyHz, g_config.volumePercent, g_config.sampleRate)) {
        g_remainingSeconds = durationSeconds;
        g_trayState = TrayState::ManualPlaying;
        g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
    }
}

void StopManualPlayback(HWND /*hWnd*/) {
    g_audioPlayer.Stop();
    g_remainingSeconds = 0;
    g_trayState = TrayState::Idle;
    g_trayManager.UpdateState(g_trayState, g_config.mode);
}

void HandleTrayClick(HWND hWnd) {
    if (g_config.mode == AppMode::FullAuto) {
        // In Full Auto mode, clicking switches to Manual Mode (Red 2h)
        StartManualPlayback(hWnd, g_config.manualDurationMinutes * 60);
    } else {
        // In Manual Mode, toggle playback or return to Full Auto
        if (g_audioPlayer.IsPlaying()) {
            StopManualPlayback(hWnd);
        } else {
            StartManualPlayback(hWnd, g_config.manualDurationMinutes * 60);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == g_uTaskbarRestartMsg && g_uTaskbarRestartMsg != 0) {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);
        g_trayManager.Initialize(hWnd, WM_TRAY_CALLBACK, hInst);
        g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
        return 0;
    }

    switch (message) {
    case WM_TRAY_CALLBACK:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
            HandleTrayClick(hWnd);
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            g_trayManager.ShowContextMenu(hWnd, g_config.mode, g_audioPlayer.IsPlaying());
            break;
        }
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_TRAY_MODE_AUTO:
            SwitchToMode(hWnd, AppMode::FullAuto);
            break;

        case ID_TRAY_MODE_MANUAL:
            SwitchToMode(hWnd, AppMode::Manual);
            break;

        case ID_TRAY_MANUAL_TOGGLE:
            if (g_config.mode == AppMode::Manual) {
                if (g_audioPlayer.IsPlaying()) {
                    StopManualPlayback(hWnd);
                } else {
                    StartManualPlayback(hWnd, g_config.manualDurationMinutes * 60);
                }
            } else {
                // In Auto mode, stop any currently playing tone
                g_audioPlayer.Stop();
                g_remainingSeconds = 0;
                g_trayState = TrayState::Idle;
                g_trayManager.UpdateState(g_trayState, g_config.mode);
            }
            break;

        case ID_TRAY_RESET_TIMER:
            if (g_config.mode == AppMode::Manual) {
                StartManualPlayback(hWnd, g_config.manualDurationMinutes * 60);
            } else {
                g_remainingSeconds = g_config.autoTimeoutMinutes * 60;
                g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
            }
            break;

        case ID_TRAY_OPEN_CONFIG: {
            std::wstring path = AppConfig::GetConfigPath();
            ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            break;
        }

        case ID_TRAY_RELOAD_CONFIG: {
            g_config.Load();
            if (g_audioPlayer.IsPlaying()) {
                g_audioPlayer.Start(g_config.frequencyHz, g_config.volumePercent, g_config.sampleRate);
            }
            g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
            break;
        }

        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;
    }

    case WM_TIMER:
        if (wParam == TIMER_TICK_ID) {
            if (g_config.mode == AppMode::FullAuto) {
                // Full Auto logic: check if any other app is playing sound
                bool audioDetected = g_audioDetector.IsAudioPlayingFromOtherApps(g_config.peakThreshold);

                if (audioDetected) {
                    // Audio from streamer/game/video detected!
                    if (!g_audioPlayer.IsPlaying()) {
                        g_audioPlayer.Start(g_config.frequencyHz, g_config.volumePercent, g_config.sampleRate);
                    }
                    // Reset 5-minute timeout
                    g_remainingSeconds = g_config.autoTimeoutMinutes * 60;
                    g_trayState = TrayState::AutoPlaying;
                    g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);

                } else {
                    // No other sound currently playing: countdown 5-minute timeout
                    if (g_trayState == TrayState::AutoPlaying) {
                        g_remainingSeconds--;
                        if (g_remainingSeconds <= 0) {
                            // 5 minutes of total silence: stop keep-alive tone
                            g_audioPlayer.Stop();
                            g_remainingSeconds = 0;
                            g_trayState = TrayState::Idle;
                            g_trayManager.UpdateState(g_trayState, g_config.mode);
                        } else {
                            g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
                        }
                    }
                }

            } else {
                // Manual Mode logic: countdown 2-hour duration
                if (g_trayState == TrayState::ManualPlaying && g_remainingSeconds > 0) {
                    g_remainingSeconds--;
                    if (g_remainingSeconds <= 0) {
                        g_audioPlayer.Stop();
                        g_remainingSeconds = 0;
                        g_trayState = TrayState::Idle;
                        g_trayManager.UpdateState(g_trayState, g_config.mode);
                    } else {
                        g_trayManager.UpdateState(g_trayState, g_config.mode, g_remainingSeconds);
                    }
                }
            }
        }
        break;

    case WM_DESTROY:
        g_audioPlayer.Stop();
        g_audioDetector.Cleanup();
        g_trayManager.Remove();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // Attach to default interactive desktop
    HDESK hDesk = OpenDesktopW(L"default", 0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE |
        DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD |
        DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_WRITEOBJECTS);
    if (hDesk) {
        SetThreadDesktop(hDesk);
    }

    // Single-instance enforcement
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"LogitechKeepAlive_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindowW(WINDOW_CLASS_NAME, NULL);
        if (existingWnd) {
            PostMessageW(existingWnd, WM_COMMAND, ID_TRAY_MANUAL_TOGGLE, 0);
        }
        if (hMutex) CloseHandle(hMutex);
        if (hDesk) CloseDesktop(hDesk);
        return 0;
    }

    // Initialize COM for WASAPI session monitor
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Load or initialize config.ini
    g_config.Load();

    // Register Explorer restart notification message
    g_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");

    // Register hidden window class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    if (!RegisterClassExW(&wcex)) {
        if (hMutex) CloseHandle(hMutex);
        if (hDesk) CloseDesktop(hDesk);
        CoUninitialize();
        return 1;
    }

    // Create top-level hidden window
    HWND hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW, WINDOW_CLASS_NAME, L"Logitech Keep-Alive Listener",
        WS_POPUP, 0, 0, 0, 0,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        if (hMutex) CloseHandle(hMutex);
        if (hDesk) CloseDesktop(hDesk);
        CoUninitialize();
        return 1;
    }

    // Initialize audio detector
    g_audioDetector.Initialize();

    // Initialize tray icon
    if (!g_trayManager.Initialize(hWnd, WM_TRAY_CALLBACK, hInstance)) {
        DestroyWindow(hWnd);
        if (hMutex) CloseHandle(hMutex);
        if (hDesk) CloseDesktop(hDesk);
        CoUninitialize();
        return 1;
    }

    g_trayState = TrayState::Idle;
    g_trayManager.UpdateState(g_trayState, g_config.mode);

    // Start 1-second background tick timer for audio detection and countdown
    SetTimer(hWnd, TIMER_TICK_ID, 1000, NULL);

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(hWnd, TIMER_TICK_ID);
    CoUninitialize();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    if (hDesk) {
        CloseDesktop(hDesk);
    }

    return (int)msg.wParam;
}
