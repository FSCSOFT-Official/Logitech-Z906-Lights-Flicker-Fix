#pragma once
#include <windows.h>
#include "Config.h"

enum class TrayState {
    Idle,          // Amber: Standby (stopped / waiting for audio)
    AutoPlaying,   // Green: Automated keep-alive active (5m timeout)
    ManualPlaying  // Red: Manual keep-alive active (2h countdown)
};

class TrayManager {
public:
    TrayManager();
    ~TrayManager();

    bool Initialize(HWND hWnd, UINT callbackMessage, HINSTANCE hInstance);
    void UpdateState(TrayState state, AppMode currentMode, int remainingSeconds = -1);
    void Remove();
    void ShowContextMenu(HWND hWnd, AppMode currentMode, bool isPlaying);

private:
    NOTIFYICONDATAW m_nid = {};
    HICON m_hIconIdle = nullptr;
    HICON m_hIconActive = nullptr;
    HICON m_hIconAuto = nullptr;
    TrayState m_currentState = TrayState::Idle;
    bool m_isAdded = false;
};
