# Reverse-engineering data

The Windows captures record Synapse initialization. Linux data records command replay and physical control testing.

| File | Contents |
| --- | --- |
| `windows/startup-control.pcapng` | USB control and interrupt packets from Synapse startup |
| `windows/comparison-control.pcapng` | A second Synapse initialization capture |
| `windows/*-frames.csv` | Filtered packet numbers mapped to the originals |
| `windows/provenance.json` | Original and filtered capture SHA-256 hashes |
| `linux/initialization.json` | Exact fader query, enable, replies, and first working report |
| `linux/fader-sweep.jsonl` | Relative times and raw reports from a USB reconnect and manual control sweep |
| `hid-report-descriptor.hex` | Descriptor read from the Linux HID node |

The published captures retain USB control and interrupt traffic. Audio transfers and USB string descriptors are excluded. The original captures remain outside this repository.

## Initialization packets

Packet numbers in the first three columns refer to the published files.

| Capture | Query | Enable | First fader report | Delay after Enable |
| --- | --- | --- | --- | --- |
| Startup | 200 | 204 | 206 | 4.985 ms |
| Comparison | 208 | 212 | 214 | 4.016 ms |

Those packets were originally 228, 232, and 234 in the startup capture, and 236, 240, and 242 in the comparison capture. Both send the same enable request. See [protocol notes](../docs/protocol.md) for the byte layout.

The Linux sweep contains 392 control reports. All four faders reached both 0 and 100. The person operating the device confirmed that Master, Chat, Music, and Mic responded correctly and that the mute colors changed correctly.

## Reproduce the filtered capture

Requires Wireshark's `tshark` command.

```sh
./research/tools/filter-capture.sh original.pcap filtered.pcapng
```

The script also writes a frame mapping CSV. Check the published files with:

```sh
cd research
sha256sum --check SHA256SUMS
```

For another Linux control capture, leave the driver running so it enables fader reporting:

```sh
razer-mixer-driver monitor --duration 60 > controls.jsonl
razer-mixer-driver descriptor > descriptor.hex
```

These commands read the HID device without changing its feature state.
