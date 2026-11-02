# Modes and the frame pipeline

How the ADS7952 picks channels, why a result arrives two frames after its command, and which settings need a
mode frame before the chip applies them.

## Three channel modes

| Mode | Channels | Driver calls | Use it for |
| --- | --- | --- | --- |
| Manual | The one the host names in each frame | `ReadChannel()`, `EnterManualMode()`, `RawManualFrames()` | Reading a channel on demand |
| Auto-1 | Any subset, in ascending order, from a 12-bit mask | `ProgramAuto1Channels()`, `EnterAuto1Mode()`, `ReadAllChannels()` | Scanning a fixed set of channels |
| Auto-2 | CH0 up to a programmed last channel | `ProgramAuto2LastChannel()`, `EnterAuto2Mode()` | Scanning a contiguous block |

`EnsureInitialized()` programs the Auto-1 mask (`ADS7952_CFG::DEFAULT_AUTO1_MASK`, all 12 channels) and the
Auto-2 last channel (`DEFAULT_AUTO2_LAST_CH`, CH11) and leaves the chip in Auto-1. `ReadChannel()` moves it to
Manual; `ReadAllChannels()` moves it back to Auto-1 and restarts the sequence.

## The command word

Every frame carries one 16-bit command on SDI. In Manual, Auto-1 and Auto-2 it is the mode control register:

<div class="hf-registers" markdown>

| Bits | Field | Meaning |
| --- | --- | --- |
| `DI15:12` | Mode | `0001` Manual, `0010` Auto-1, `0011` Auto-2, `0000` continue in the current mode |
| `DI11` | Program | 1: apply DI06:00 in this frame (Auto-1, Auto-2: DI10:00); 0: keep the previous values |
| `DI10` | Reset counter (Auto) | 1: restart the sequence at its first channel |
| `DI10:07` | Channel (Manual) | Channel to select next, applied whatever DI11 says |
| `DI06` | Range | 0: 0 to VREF; 1: 0 to 2 × VREF |
| `DI05` | Power down | 1: power down at the 16th SCLK falling edge |
| `DI04` | Output select | 0: DO15:12 carries the channel address; 1: the GPIO input levels |
| `DI03:00` | GPIO data | Levels of the GPIOs configured as outputs |

</div>

Other values of DI15:12 open the program registers: `0100` GPIO, `1000` and `1001` the Auto-1 mask and Auto-2 last
channel, `1100` to `1110` the alarm thresholds of channels 0-3, 4-7 and 8-11. The constants are in
[`inc/ads7952_registers.hpp`](../inc/ads7952_registers.hpp); the driver keeps DI04 at 0 so every reply is tagged.

## The reply word and its latency

The reply on SDO is the conversion of the input sampled at this frame's CS falling edge:

<div class="hf-registers" markdown>

| Bits | Field |
| --- | --- |
| `DO15:12` | Channel address of the conversion (DI04 = 0) |
| `DO11:00` | 12-bit count, MSB first |

</div>

A channel named in frame N is selected at the end of frame N, acquired during frame N + 1 and sampled at the start
of frame N + 2. Its result therefore comes back in frame N + 2, and the replies in frames N and N + 1 belong to
channels chosen earlier.

The driver never relies on counting frames. `ReadChannel()` sends the Manual frame, then clocks continue frames
until DO15:12 equals the channel it asked for (at most `ADS7952_CFG::MANUAL_READ_MAX_FRAMES`, default 4), and
returns `Error::Timeout` if it never does. `ReadAllChannels()` restarts Auto-1 and keeps each reply whose address
is in the mask, until every programmed channel has arrived or the budget (one frame per channel plus
`READ_ALL_MAX_EXTRA_FRAMES`, default 4) runs out.

## Settings that need a mode frame

A continue frame (DI15:12 = `0000`) keeps the mode, and the chip **ignores DI11:00** in it (datasheet Table 7).
`SetRange()`, `SetPowerDown()` and `SetGPIOOutputs()` update the driver's state and send a continue frame, so the
chip applies the new range, power-down or GPIO levels only with the next frame that has DI11 = 1:

| Call | Sends DI11 = 1 | Applies a pending range, power-down or GPIO change |
| --- | :---: | :---: |
| `EnterManualMode()`, `EnterAuto1Mode()`, `EnterAuto2Mode()` | yes | yes |
| `ReadAllChannels()` | yes | yes |
| `ReadChannel()`, `RawManualFrames()` | no | no |
| `SetRange()`, `SetPowerDown()`, `SetGPIOOutputs()` | no | no |

Call one of the `Enter...Mode()` functions after changing these settings, as the [API guide](api-guide.md#configure)
does. `CountToVoltage()` switches to the new reference at once, so until the chip has latched the range, counts
are converted with the wrong full scale.

## Programming sequences

| Register | Frames | Driver call |
| --- | --- | --- |
| Auto-1 mask | 2: `1000`, then the mask in DI11:00 | `ProgramAuto1Channels(mask)` |
| Auto-2 last channel | 1: `1001` with the channel in DI09:06 | `ProgramAuto2LastChannel(ch)` |
| GPIO configuration | 1: `0100` with the functions and directions | `ProgramGPIO(config)` |
| One alarm threshold | 3: group, threshold with the exit bit, one continue frame | `ProgramAlarm()`, `ProgramAlarmVoltage()` |

The chip keeps converting in its current mode while it is being programmed, and the replies stay valid; the driver
discards them.

Next: [Architecture](architecture.md) shows where this logic lives in the code.
