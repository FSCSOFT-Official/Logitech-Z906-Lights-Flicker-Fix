#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

class AudioDetector {
public:
    AudioDetector();
    ~AudioDetector();

    bool Initialize();
    void Cleanup();
    bool IsAudioPlayingFromOtherApps(float peakThreshold = 0.001f);

private:
    IMMDeviceEnumerator* m_pEnumerator = nullptr;
    DWORD m_myPid = 0;
};
