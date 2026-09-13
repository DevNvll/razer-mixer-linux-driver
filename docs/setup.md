# Setup

The kernel handles USB audio. This driver reads the mixer's HID controls and applies them to PipeWire through `pactl` and `wpctl`.

## USB access

The driver needs read/write access to the mixer's `/dev/hidraw*` node. If it reports permission denied, install the supplied udev rule and reconnect the mixer.

```sh
sudo install -m 0644 packaging/99-razer-mixer.rules /etc/udev/rules.d/99-razer-mixer.rules
sudo udevadm control --reload-rules
```

Run the driver as your desktop user. It needs that user's audio server.

## Build and install

```sh
./scripts/install.sh
```

The default prefix is `~/.local`. Set `PREFIX` to change it, or `JOBS` to change build parallelism. Make sure the prefix's `bin` directory is on your `PATH`.

For a build without installation:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/razer-mixer-driver init
./build/razer-mixer-driver run
```

Open `./build/razer-mixer` in another terminal. Closing the app leaves the driver running.

## Audio devices and routing

The default settings use the mixer's analog stereo output and input. Inspect available nodes with:

```sh
razer-mixer-driver devices
```

If your audio profile uses different names, set `output` and `microphone` in `~/.config/razer-mixer/config.json`. Select the mixer's hardware output as your desktop's default output. Master controls audio sent to that output.

The driver creates `Razer Chat` and `Razer Music` virtual outputs and feeds both into the hardware output. Discord and Vesktop use Chat by default. Spotify uses Music. The Applications page saves overrides, including System for direct hardware output. Routing updates within about two seconds.

## App controls

The Mixer page displays current audio levels. Change volume with the physical faders. They have no motors, so the app does not offer draggable faders. The first report after connection establishes a baseline and preserves existing audio levels until a fader moves.

Mute buttons work in the app and on the device. Lighting provides full color pickers, hex entry, presets, and brightness. Accepting a color saves it; cancelling leaves it unchanged. The active color also sets the app accent. Use Ctrl+1, Ctrl+2, and Ctrl+3 to switch pages.

Settings follow `XDG_CONFIG_HOME`. Virtual channel levels and mute states follow `XDG_STATE_HOME`. The driver restores those channel settings when it recreates its outputs.

## Troubleshooting

```sh
razer-mixer-driver status
systemctl --user status razer-mixer-bridge.service
journalctl --user -u razer-mixer-bridge.service -n 50
```

A connected mixer should report `device.connected` and `device.fadersReady` as true. If only the second is false, check the reported error and reconnect USB. Only one mixer is supported at a time.

The driver discovers the HID node again after a disconnect. Run only one driver instance. The service and a foreground driver share a lock.

```sh
./scripts/uninstall.sh
```

Uninstalling stops the service and removes the installed app and executables. It keeps your settings. The udev rule remains installed.
