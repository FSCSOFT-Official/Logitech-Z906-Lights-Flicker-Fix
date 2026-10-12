#include "TrayManager.h"
#include "resource.h"
#include <cwchar>
#include <cstdio>

TrayManager::TrayManager() {
    ZeroMemory(&m_nid, sizeof(NOTIFYICONDATAW));
}

TrayManager::~TrayManager() {
    Remove();
    if (m_hIconIdle) DestroyIcon(m_hIconIdle);
    if (m_hIconActive) DestroyIcon(m_hIconActive);
    if (m_hIconAuto) DestroyIcon(m_hIconAuto);
}

bool TrayManager::Initialize(HWND hWnd, UINT callbackMessage, HINSTANCE hInstance) {
    int iconCx = GetSystemMetrics(SM_CXSMICON);
    int iconCy = GetSystemMetrics(SM_CYSMICON);

    m_hIconIdle = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON_IDLE), IMAGE_ICON, iconCx, iconCy, LR_DEFAULTCOLOR);
    m_hIconActive = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON_ACTIVE), IMAGE_ICON, iconCx, iconCy, LR_DEFAULTCOLOR);
    m_hIconAuto = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON_AUTO), IMAGE_ICON, iconCx, iconCy, LR_DEFAULTCOLOR);

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = callbackMessage;
    m_nid.hIcon = m_hIconIdle ? m_hIconIdle : LoadIcon(NULL, IDI_APPLICATION);

    wcscpy_s(m_nid.szTip, L"Logitech Keep-Alive: Standby (Amber)");

    for (int retry = 0; retry < 10; ++retry) {
        if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
            m_isAdded = true;
            break;
        }
        Sleep(200);
    }

    m_currentState = TrayState::Idle;
    return m_isAdded;
}

void TrayManager::UpdateState(TrayState state, AppMode currentMode, int remainingSeconds) {
    m_currentState = state;
    m_nid.uFlags = NIF_ICON | NIF_TIP;

    if (state == TrayState::AutoPlaying) {
        // Green icon: Full Auto keep-alive playing
        m_nid.hIcon = m_hIconAuto ? m_hIconAuto : m_hIconActive;

        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;
        swprintf_s(m_nid.szTip, L"Logitech Keep-Alive: FULL AUTO (Green)\nAudio active (Timeout in: %02dm %02ds)\nLeft-click to toggle mode", minutes, seconds);

    } else if (state == TrayState::ManualPlaying) {
        // Red icon: Manual 2-hour keep-alive playing
        m_nid.hIcon = m_hIconActive ? m_hIconActive : m_hIconIdle;

        if (remainingSeconds >= 0) {
            int hours = remainingSeconds / 3600;
            int minutes = (remainingSeconds % 3600) / 60;
            int seconds = remainingSeconds % 60;

            if (hours > 0) {
                swprintf_s(m_nid.szTip, L"Logitech Keep-Alive: MANUAL (Red)\nTime remaining: %dh %02dm\nLeft-click to stop", hours, minutes);
            } else {
                swprintf_s(m_nid.szTip, L"Logitech Keep-Alive: MANUAL (Red)\nTime remaining: %02dm %02ds\nLeft-click to stop", minutes, seconds);
            }
        } else {
            wcscpy_s(m_nid.szTip, L"Logitech Keep-Alive: MANUAL (Red)\nContinuous (No timeout)\nLeft-click to stop");
        }

    } else {
        // Amber icon: Idle / Standby
        m_nid.hIcon = m_hIconIdle;

        if (currentMode == AppMode::FullAuto) {
            wcscpy_s(m_nid.szTip, L"Logitech Keep-Alive: FULL AUTO (Amber)\nMonitoring system audio...\nLeft-click to toggle mode");
        } else {
            wcscpy_s(m_nid.szTip, L"Logitech Keep-Alive: MANUAL (Amber)\nLeft-click to start 2h keep-alive");
        }
    }

    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TrayManager::Remove() {
    if (m_isAdded) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isAdded = false;
    }
}

void TrayManager::ShowContextMenu(HWND hWnd, AppMode currentMode, bool isPlaying) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // Mode Selection Header
    UINT autoCheck = (currentMode == AppMode::FullAuto) ? (MF_CHECKED) : (MF_UNCHECKED);
    UINT manualCheck = (currentMode == AppMode::Manual) ? (MF_CHECKED) : (MF_UNCHECKED);

    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING | autoCheck, ID_TRAY_MODE_AUTO, L"Full Auto Mode (Green on audio, 5m timeout)");
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING | manualCheck, ID_TRAY_MODE_MANUAL, L"Manual Mode (Red, 2-hour click toggle)");
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

    // Context actions based on current mode & state
    if (currentMode == AppMode::Manual) {
        if (isPlaying) {
            InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_MANUAL_TOGGLE, L"Stop Keep-Alive");
            InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_RESET_TIMER, L"Restart 2-Hour Timer");
            SetMenuDefaultItem(hMenu, ID_TRAY_MANUAL_TOGGLE, FALSE);
        } else {
            InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_MANUAL_TOGGLE, L"Start Keep-Alive (2 Hours)");
            SetMenuDefaultItem(hMenu, ID_TRAY_MANUAL_TOGGLE, FALSE);
        }
    } else {
        // Full Auto Mode actions
        if (isPlaying) {
            InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_MANUAL_TOGGLE, L"Stop Audio (Return to Amber Standby)");
        } else {
            InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, L"Auto-Detection Active (Listening...)");
        }
    }

    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN_CONFIG, L"Open config.ini");
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_RELOAD_CONFIG, L"Reload config.ini");
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RIGHTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}
