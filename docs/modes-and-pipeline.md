# Modes and the frame pipeline

How the ADS7952 picks channels, why a result arrives two frames after its command, and how the range, power-down
and GPIO settings reach the chip.

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

## Range, power-down and GPIO outputs

The range (DI06), power-down (DI05) and GPIO output levels (DI03:00) live in the mode-control register. The chip
takes them only from a Manual, Auto-1 or Auto-2 frame with the programming bit DI11 = 1 (datasheet Tables 1, 2
and 5); with DI11 = 0 it keeps the previous values, and a continue frame (DI15:12 = `0000`) ignores DI11:00
altogether (Table 7).

`SetRange()`, `SetPowerDown()` and `SetGPIOOutputs()` therefore send two frames:

1. A mode frame for the mode the chip is in, with DI11 = 1 and the new bits: Manual re-selects the current channel,
   Auto-1 and Auto-2 keep counting (DI10 = 0, no counter reset). The chip latches the bits at the end of this
   frame, and the driver updates its own copy (`GetRange()`, `GetActiveVref()`, ...) at the same point.
2. One continue frame. A range written in frame N applies to the input acquired in frame N + 1 (Figure 50), GPIO
   outputs change on the CS falling edge of frame N + 1 (section 8.1), and DI05 = 1 powers the chip down on the
   16th SCLK of frame N + 1 (section 8.3.2). The reply of this frame is the last conversion made with the old
   range, and the driver drops it.

When the call returns, the chip runs with the new setting and every result clocked out from then on was converted
with it, so `CountToVoltage()` and the voltages in `ReadResult` / `ChannelReadings` always use the full scale the
chip used. In Auto-1 and Auto-2 the two frames advance the channel counter by two, which `ReadAllChannels()`
resets anyway.

| Call | Sends DI11 = 1 with the range, power-down and GPIO bits |
| --- | :---: |
| `SetRange()`, `SetPowerDown()`, `SetGPIOOutputs()` | yes, then one flush frame |
| `EnterManualMode()`, `EnterAuto1Mode()`, `EnterAuto2Mode()`, `ReadAllChannels()` | yes (the bits the driver holds) |
| `ReadChannel()`, `RawManualFrames()` | no: DI11 = 0, the chip keeps its settings |

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
