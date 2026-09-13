# Reverse-engineering notes

Windows Synapse captures identified the command that enables the Razer Audio Mixer's physical faders. Linux replay and USB reconnect tests confirmed the behavior described in the [protocol notes](../docs/protocol.md).

## Fader initialization

Before initialization, mute buttons sent reports but all four fader bytes stayed at zero. The trailing `04` byte was present in both working and uninitialized reports.

Synapse queried class `0f`, command `95`, then enabled reporting with command `15`. The query returned `00 00` before enabling and `00 01` afterward. Both requests used a zero checksum byte, so the driver preserves their exact bytes in [Protocol.cpp](../src/Protocol.cpp).

| Observation | Query frame | Enable frame | First fader report | Delay after Enable |
| --- | --- | --- | --- | --- |
| Windows startup | 228 | 232 | 234 | 4.985 ms |
| Windows comparison | 236 | 240 | 242 | 4.016 ms |
| Linux replay | n/a | n/a | `09 00 00 64 64 64 64 04` | 4.095 ms |

Frame numbers refer to the original Windows captures. Both sent the same enable request.

## Physical control checks

A Linux USB reconnect and manual sweep produced 392 control reports. Every fader reached 0 and 100. The operator confirmed that Master controlled the whole output, Chat controlled its assigned apps, Music controlled Spotify, and Mic controlled the microphone input.

Each channel's mute button changed its audio mute state. Active and muted RGB pairs made the button lights follow that state. The dedicated mic button controlled the same input as channel 4.

After the C++ port, another USB reconnect and manual test confirmed all four faders and mute buttons. The service reconnected without restarting.

## Driver behavior

The first valid position report establishes a baseline. It does not change audio levels. Later reports update only the faders that moved, and button handling uses rising edges to avoid repeated toggles while held.

Master controls the hardware output. Chat and Music use virtual outputs routed into it. This lets Master lower the full mix while the other two faders adjust their own channels.

The [protocol reference](../docs/protocol.md) documents report layouts, initialization bytes, lighting zones, brightness, and mute feedback. [Setup](../docs/setup.md) covers installation and diagnostics.
