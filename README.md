# Razer Audio Mixer for Linux

A C++ driver and standalone QML app for the Razer Audio Mixer, USB `1532:053e`.

| Fader | Controls |
| --- | --- |
| 1 | Master output, including Chat and Music |
| 2 | Chat, with Discord routed here by default |
| 3 | Music, with Spotify routed here by default |
| 4 | Microphone input |

Physical mute buttons work with configurable active and muted colors. The app shows read-only faders and lets you mute channels, assign apps, and choose lighting colors and brightness. Its accent follows the active color.

## Install

Requires Linux, PipeWire with PulseAudio compatibility, WirePlumber, `pactl`, `wpctl`, systemd user services, CMake 3.21+, a C++17 compiler, and Qt 6.4+ development packages for Quick, Controls, Dialogs, and Test.

```sh
./scripts/install.sh
razer-mixer
```

The installer builds both executables, runs tests, and starts the driver at login. See [setup](docs/setup.md) for USB permissions and audio device selection.

The repository includes [protocol notes](docs/protocol.md) and [reverse-engineering captures](research/README.md). This is an unofficial driver. Firmware updates and microphone DSP settings are outside its scope.
