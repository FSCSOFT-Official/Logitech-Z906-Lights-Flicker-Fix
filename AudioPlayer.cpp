#include "AudioPlayer.h"
#include <cmath>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

static const double PI = 3.14159265358979323846;

AudioPlayer::AudioPlayer() {
    ZeroMemory(&m_waveHdr, sizeof(WAVEHDR));
}

AudioPlayer::~AudioPlayer() {
    Stop();
}

bool AudioPlayer::Start(double frequencyHz, double volumePercent, int sampleRate) {
    Stop();

    if (sampleRate <= 0) sampleRate = 48000;
    if (frequencyHz <= 0.0) frequencyHz = 23999.0;
    if (volumePercent <= 0.0) volumePercent = 1.0;

    // Minimum volume calculation:
    // At VolumePercent = 1.0, amplitude is ~32 (out of 32767), roughly -60 dBFS
    // Ensuring amplitude is at least 1 (digital non-zero)
    double normalizedVol = volumePercent / 100.0;
    short amplitude = static_cast<short>(std::round(32767.0 * normalizedVol * 0.10));
    if (amplitude < 1) amplitude = 1;

    // 1-second stereo buffer (sampleRate samples)
    int totalSamples = sampleRate;
    m_buffer.resize(totalSamples * 2);

    double twoPi = 2.0 * PI;
    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(sampleRate);
        double val = std::sin(twoPi * frequencyHz * t);
        short sampleVal = static_cast<short>(val * amplitude);
        m_buffer[i * 2 + 0] = sampleVal; // Left channel
        m_buffer[i * 2 + 1] = sampleVal; // Right channel
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2; // Stereo
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    MMRESULT res = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (res != MMSYSERR_NOERROR) {
        m_hWaveOut = nullptr;
        return false;
    }

    ZeroMemory(&m_waveHdr, sizeof(WAVEHDR));
    m_waveHdr.lpData = reinterpret_cast<LPSTR>(m_buffer.data());
    m_waveHdr.dwBufferLength = static_cast<DWORD>(m_buffer.size() * sizeof(short));
    m_waveHdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    m_waveHdr.dwLoops = 0xFFFFFFFF; // Infinite loop

    res = waveOutPrepareHeader(m_hWaveOut, &m_waveHdr, sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR) {
        waveOutClose(m_hWaveOut);
        m_hWaveOut = nullptr;
        return false;
    }

    res = waveOutWrite(m_hWaveOut, &m_waveHdr, sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR) {
        waveOutUnprepareHeader(m_hWaveOut, &m_waveHdr, sizeof(WAVEHDR));
        waveOutClose(m_hWaveOut);
        m_hWaveOut = nullptr;
        return false;
    }

    m_isPlaying = true;
    return true;
}

void AudioPlayer::Stop() {
    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut);
        waveOutUnprepareHeader(m_hWaveOut, &m_waveHdr, sizeof(WAVEHDR));
        waveOutClose(m_hWaveOut);
        m_hWaveOut = nullptr;
    }
    ZeroMemory(&m_waveHdr, sizeof(WAVEHDR));
    m_isPlaying = false;
}
