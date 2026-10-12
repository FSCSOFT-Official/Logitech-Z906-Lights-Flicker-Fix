#pragma once
#include <string>

enum class AppMode {
    FullAuto = 0,
    Manual = 1
};

struct AppConfig {
    double frequencyHz = 23999.0;
    double volumePercent = 1.0;
    int sampleRate = 48000;
    AppMode mode = AppMode::FullAuto;
    int autoTimeoutMinutes = 5;
    int manualDurationMinutes = 120;
    float peakThreshold = 0.001f;

    static std::wstring GetConfigPath();
    bool Load(const std::wstring& path = L"");
    bool Save(const std::wstring& path = L"") const;
};
