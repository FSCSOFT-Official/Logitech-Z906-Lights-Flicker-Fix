#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <vector>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool Start(double frequencyHz, double volumePercent, int sampleRate = 48000);
    void Stop();
    bool IsPlaying() const { return m_isPlaying; }

private:
    HWAVEOUT m_hWaveOut = nullptr;
    WAVEHDR m_waveHdr = {};
    std::vector<short> m_buffer;
    bool m_isPlaying = false;
};
