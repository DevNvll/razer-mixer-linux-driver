# USB protocol

These notes describe commands tested on the Razer Audio Mixer, VID `1532`, PID `053e`. The [research findings](../research/README.md) summarize the Windows captures and Linux tests.

## HID reports

The tested device exposes HID on interface 6 with interrupt endpoint `0x84`. The driver discovers its hidraw node through sysfs rather than assuming a fixed device path.

Feature reports contain 64 bytes, including report ID `07`.

| Offset | Content |
| --- | --- |
| 0 | Report ID, `07` |
| 1 | Status, `00` for requests, `02` for successful replies |
| 2 | Transaction byte, usually `1f` for writes |
| 3 to 5 | Zero in the tested requests |
| 6 | Command data size |
| 7 | Command class |
| 8 | Command ID |
| 9 to 61 | Arguments, padded with zeroes |
| 62 | XOR of bytes 3 through 61, except the initialization requests below |
| 63 | Zero |

Ordinary commands use SET_FEATURE followed by GET_FEATURE. Replies must match the class and command and report success. The driver enters software control mode with class `00`, command `04`, arguments `03 00`.

## Enable fader reports

A USB reconnect can leave buttons working while all fader bytes remain zero. Synapse enables fader reporting with class `0f`, command `15`.

The following are report prefixes. Pad each to 64 bytes with zeroes, including offset 62. Preserve these captured bytes exactly; the ordinary checksum builder produces different requests.

```text
Query:   07 00 00 00 00 00 02 0f 95 00 00
Enable:  07 00 1f 00 00 00 02 0f 15 00 01
```

Query replies contain `00 00` at offsets 9 to 10 while disabled, and `00 01` while enabled. Their checksum bytes were `98` and `99` respectively.

The driver queries the state, sends Enable when needed, then queries again. A Linux replay changed the state to enabled and produced a nonzero interrupt report after 4.095 ms. Two Windows captures show the same transition after 4.985 ms and 4.016 ms.

## Faders and buttons

Interrupt report `09` has eight bytes:

```text
09 buttons 00 master chat music mic 04
```

Fader values are integer percentages from 0 through 100. The trailing `04` also appears before initialization, so it does not prove faders are ready.

| Button | Bit mask |
| --- | --- |
| Master mute | `01` |
| Chat mute | `02` |
| Music mute | `04` |
| Mic channel mute | `08` |
| Dedicated mic mute | `20` |

Each rising bit toggles that channel's mute state. Holding a button does not toggle it repeatedly. The dedicated mic button and channel 4 control the same microphone source.

The driver uses the first valid position report as its baseline. Later changes update only the faders that moved. All-zero reports are ignored as positions until reporting is enabled or a nonzero position arrives. Once enabled, zero is a valid volume.

## Lighting

Brightness uses class `0f`, command `04`, with arguments `01 zone brightness`. The tested brightness range is 0 through 100.

Static color uses class `0f`, command `02`. Arguments start with `00 zone 01 00 00 count`, followed by RGB triples. The size byte follows the observed values below; it is not always the length of the populated arguments.

| Zone | LEDs | Count | Size byte |
| --- | --- | --- | --- |
| `04` | Wordmark | 1 | `1b` |
| `05` | Four fader backgrounds | 4 | `24` |
| `06` to `09` | Individual fader foregrounds | 1 each | `1b` |
| `0a` | Channel numbers | 4 | `24` |
| `10` | Channel mute buttons | 8 | `1b` |
| `20` | Dedicated mic mute | 2 | `1e` |
| `21` | Bleep button | 2 | `1e` |

Zone `10` takes four active/muted RGB pairs. Zones `20` and `21` take one pair each. The driver holds zone `05` at zero brightness with black RGB values, so it does not illuminate the unfilled track. Other zones repeat the active color at the configured brightness.

Mute feedback uses class `08`, command `10`, arguments `00 channel muted`. Channel numbers run from 1 through 5; muted is `00` or `01`. The driver follows software mute state so the button lights also change after an app mute action.

The bleep light is configurable. Bleep audio behavior, phantom power, microphone DSP, and firmware updates are not implemented.
