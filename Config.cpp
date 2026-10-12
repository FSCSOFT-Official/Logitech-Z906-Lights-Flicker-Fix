#include "Config.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iomanip>

std::wstring AppConfig::GetConfigPath() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        path = path.substr(0, pos + 1);
    }
    path += L"config.ini";
    return path;
}

bool AppConfig::Load(const std::wstring& inPath) {
    std::wstring path = inPath.empty() ? GetConfigPath() : inPath;
    
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        Save(path);
        return true;
    }

    wchar_t buf[128] = {};

    GetPrivateProfileStringW(L"Audio", L"FrequencyHz", L"23999", buf, 128, path.c_str());
    frequencyHz = std::wcstod(buf, nullptr);
    if (frequencyHz <= 0.0 || frequencyHz > 96000.0) frequencyHz = 23999.0;

    GetPrivateProfileStringW(L"Audio", L"VolumePercent", L"1.0", buf, 128, path.c_str());
    volumePercent = std::wcstod(buf, nullptr);
    if (volumePercent <= 0.0) volumePercent = 1.0;
    if (volumePercent > 100.0) volumePercent = 100.0;

    sampleRate = GetPrivateProfileIntW(L"Audio", L"SampleRate", 48000, path.c_str());
    if (sampleRate != 44100 && sampleRate != 48000 && sampleRate != 96000 && sampleRate != 192000) {
        sampleRate = 48000;
    }

    // Mode: 0 = FullAuto, 1 = Manual
    int modeVal = GetPrivateProfileIntW(L"Mode", L"FullAutoMode", 1, path.c_str());
    mode = (modeVal != 0) ? AppMode::FullAuto : AppMode::Manual;

    autoTimeoutMinutes = GetPrivateProfileIntW(L"AutoMode", L"TimeoutMinutes", 5, path.c_str());
    if (autoTimeoutMinutes <= 0) autoTimeoutMinutes = 5;

    manualDurationMinutes = GetPrivateProfileIntW(L"ManualMode", L"DurationMinutes", 120, path.c_str());
    if (manualDurationMinutes <= 0) manualDurationMinutes = 120;

    GetPrivateProfileStringW(L"AutoMode", L"PeakThreshold", L"0.001", buf, 128, path.c_str());
    peakThreshold = std::wcstof(buf, nullptr);
    if (peakThreshold <= 0.0f) peakThreshold = 0.001f;

    return true;
}

bool AppConfig::Save(const std::wstring& inPath) const {
    std::wstring path = inPath.empty() ? GetConfigPath() : inPath;

    std::wofstream file(path);
    if (!file.is_open()) return false;

    file << L"; Logitech Z906 Lights Flicker & Audio Sleep Fix\n";
    file << L"; An automated background utility that prevents Logitech amps from sleeping on silent audio.\n\n";

    file << L"[Mode]\n";
    file << L"; 1 = Full Auto Mode (auto-detects audio, turns Green, 5m timeout)\n";
    file << L"; 0 = Manual Mode (click tray icon to turn Red for 2 hours)\n";
    file << L"FullAutoMode=" << (mode == AppMode::FullAuto ? 1 : 0) << L"\n\n";

    file << L"[Audio]\n";
    file << L"; Target frequency in Hz (e.g. 23999, 22000, 20000). Nyquist ceiling at 48kHz is 24000Hz.\n";
    file << L"FrequencyHz=" << std::fixed << std::setprecision(1) << frequencyHz << L"\n\n";

    file << L"; Volume percentage (1.0 = lowest non-zero volume, enough to trigger hardware silence gate)\n";
    file << L"VolumePercent=" << std::fixed << std::setprecision(1) << volumePercent << L"\n\n";

    file << L"; Audio output sample rate (48000 is recommended for 23999Hz)\n";
    file << L"SampleRate=" << sampleRate << L"\n\n";

    file << L"[AutoMode]\n";
    file << L"; Silence duration in minutes before stopping keep-alive when no audio is detected (default: 5)\n";
    file << L"TimeoutMinutes=" << autoTimeoutMinutes << L"\n\n";

    file << L"; Minimum audio level to detect active sound (default: 0.001)\n";
    file << L"PeakThreshold=" << std::fixed << std::setprecision(4) << peakThreshold << L"\n\n";

    file << L"[ManualMode]\n";
    file << L"; Duration in minutes when manually started via left-click or menu (default: 120 = 2 hours)\n";
    file << L"DurationMinutes=" << manualDurationMinutes << L"\n";

    file.close();
    return true;
}
