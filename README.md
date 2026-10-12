# Logitech Z906 Audio Keep-Alive Tray App

A tiny (~56 KB), zero-dependency native Windows C++ application designed to keep Logitech speaker amplifiers (such as the Logitech Z906) awake during quiet streams or audio with heavy noise gates (e.g. Twitch streams).

---

## Operating Modes

### 1. Full Auto Mode (Default)
- **Zero interaction required!**
- The app monitors whether any other application (browser, media player, game) is playing audio.
- When audio is detected:
  - Keep-alive sound (**23,999 Hz**) starts automatically.
  - The tray icon turns **Green**.
  - A **5-minute timeout** starts.
- As long as audio continues playing, the 5-minute timeout resets continuously.
- When media stops or speech pauses for 5 continuous minutes, keep-alive turns off automatically and the icon returns to **Amber**.

### 2. Manual Mode
- Left-clicking the tray icon starts keep-alive for **2 hours** (120 minutes) and then automatically stops.
- The tray icon turns **Red**.
- Left-clicking again stops playback immediately.

---

## Tray Icon Colors (Logitech Z906 6-Speaker Layout)
Modeled directly after the Logitech Z906 console display:
- **Amber / Orange**: Standby / Stopped (listening for audio).
- **Green**: Automated Keep-Alive Active (auto-detected sound, 5m timeout).
- **Red**: Manual Keep-Alive Active (2-hour countdown).

---

## How to Toggle Between Auto and Manual Mode
You can toggle between **Full Auto Mode** and **Manual Mode** in two ways:
1. **Right-click the tray icon**: Select **Full Auto Mode** or **Manual Mode** directly from the context menu (active mode has a checkmark).
2. **Left-click**: Left-clicking while in Full Auto mode instantly switches to Manual 2-hour mode.

---

## Configuration ([`config.ini`](config.ini))
```ini
[Mode]
; 1 = Full Auto Mode (auto-detects audio, turns Green, 5m timeout)
; 0 = Manual Mode (click tray icon to turn Red for 2 hours)
FullAutoMode=1

[Audio]
; Target frequency in Hz (default: 23999.0 Hz). Nyquist ceiling at 48kHz is 24000Hz.
FrequencyHz=23999.0

; Volume percentage (1.0 = lowest non-zero volume, enough to trigger hardware silence gate)
VolumePercent=1.0

; Audio output sample rate (default: 48000)
SampleRate=48000

[AutoMode]
; Silence duration in minutes before stopping keep-alive when no audio is detected (default: 5)
TimeoutMinutes=5

; Minimum audio level to detect active sound (default: 0.001)
PeakThreshold=0.001

[ManualMode]
; Duration in minutes when manually started via left-click or menu (default: 120 = 2 hours)
DurationMinutes=120
```

---

## How to Build from Source
Run `build.bat` in this folder to recompile `LogitechKeepAlive.exe` with MSVC.
