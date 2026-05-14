# Star Wars ASCIIMATION for LilyGO T-Display S3

A standalone Star Wars ASCIIMATION player for the LilyGO T-Display S3 (ESP32-S3) device.

This project plays the classic ASCII Star Wars animation directly on the built-in TFT display using an optimized embedded movie format and custom playback engine designed for low flicker and fast rendering.

## Features

- Embedded Star Wars ASCIIMATION movie
- Optimized playback for LilyGO T-Display S3
- Persistent resume position after reboot
- Pause menu with progress bar
- Configurable text colors with persistent storage
- Fast-forward support
- Frame buffering to reduce flicker
- No SD card required
- No filesystem upload required

## Hardware

Tested on:

- LilyGO T-Display S3
- ESP32-S3
- 320x170 TFT display

## Controls

USB connector facing RIGHT:

| Button | Action |
|---|---|
| Short A | Jump backward 30 seconds |
| Short B | Jump forward 1 minute |
| Long B | Temporary fast-forward while held |
| Long A | Pause movie |

On main screen:

| Button | Action |
|---|---|
| Short A/B | Start movie once |
| Long A/B | Start movie in continuous loop mode |

While paused:

| Button | Action |
|---|---|
| Short A/B | Resume playback |
| Long A | Restart movie from beginning |
| Long B | Change text color |

## Playback Resume

The current movie position is automatically saved every few seconds.

If the device restarts or loses power, playback resumes from the last saved position after leaving the welcome screen.

## Installation

1. Install Arduino IDE
2. Install ESP32 board support
3. Install TFT_eSPI library
4. Copy all project files into the same sketch folder
5. Compile and upload

No filesystem upload step is required because the movie is embedded directly into flash memory.

## Libraries

Required:

- TFT_eSPI
- Preferences (included with ESP32 core)

## Credits

Original Star Wars ASCIIMATION created by Simon Jansen

https://www.asciimation.co.nz/

This project adapts the original ASCII animation for the LilyGO T-Display S3 hardware platform.

## Notes

This is a fan-made hardware adaptation for ESP32/LilyGO devices.

Star Wars is property of Lucasfilm/Disney.

## License

MIT License
