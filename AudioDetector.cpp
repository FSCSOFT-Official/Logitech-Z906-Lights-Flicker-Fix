#include "AudioDetector.h"

#pragma comment(lib, "ole32.lib")

AudioDetector::AudioDetector() {
    m_myPid = GetCurrentProcessId();
}

AudioDetector::~AudioDetector() {
    Cleanup();
}

bool AudioDetector::Initialize() {
    if (m_pEnumerator) return true;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&m_pEnumerator
    );

    return SUCCEEDED(hr) && (m_pEnumerator != nullptr);
}

void AudioDetector::Cleanup() {
    if (m_pEnumerator) {
        m_pEnumerator->Release();
        m_pEnumerator = nullptr;
    }
}

bool AudioDetector::IsAudioPlayingFromOtherApps(float peakThreshold) {
    if (!m_pEnumerator && !Initialize()) {
        return false;
    }

    IMMDevice* pDevice = nullptr;
    HRESULT hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
    if (FAILED(hr) || !pDevice) {
        // Device might have changed or disconnected; force re-enumerator on next call
        Cleanup();
        return false;
    }

    IAudioSessionManager2* pSessionManager = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
    if (FAILED(hr) || !pSessionManager) {
        pDevice->Release();
        return false;
    }

    IAudioSessionEnumerator* pSessionEnum = nullptr;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnum);
    if (FAILED(hr) || !pSessionEnum) {
        pSessionManager->Release();
        pDevice->Release();
        return false;
    }

    int sessionCount = 0;
    pSessionEnum->GetCount(&sessionCount);

    bool otherAudioActive = false;

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* pSessionCtrl = nullptr;
        if (SUCCEEDED(pSessionEnum->GetSession(i, &pSessionCtrl)) && pSessionCtrl) {
            IAudioSessionControl2* pSessionCtrl2 = nullptr;
            if (SUCCEEDED(pSessionCtrl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionCtrl2)) && pSessionCtrl2) {
                DWORD pid = 0;
                pSessionCtrl2->GetProcessId(&pid);

                // Exclude ourselves
                if (pid != m_myPid) {
                    IAudioMeterInformation* pMeter = nullptr;
                    if (SUCCEEDED(pSessionCtrl->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&pMeter)) && pMeter) {
                        float peak = 0.0f;
                        if (SUCCEEDED(pMeter->GetPeakValue(&peak))) {
                            if (peak >= peakThreshold) {
                                otherAudioActive = true;
                            }
                        }
                        pMeter->Release();
                    }
                }
                pSessionCtrl2->Release();
            }
            pSessionCtrl->Release();
        }

        if (otherAudioActive) {
            break;
        }
    }

    pSessionEnum->Release();
    pSessionManager->Release();
    pDevice->Release();

    return otherAudioActive;
}
