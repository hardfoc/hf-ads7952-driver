# API guide

How to do the common jobs with the driver: initialise, configure, read, program alarms and GPIOs, and handle
errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Adc` standing for `ads7952::ADS7952<MySpi>`.
The [API reference](api-reference.md) lists every member.

## The shape of the API

One `ADS7952<SpiType>` object per chip. It owns no hardware: it holds a reference to your port and mirrors what it
has programmed into the chip.

| Group | Calls | Returns |
| --- | --- | --- |
| Initialise | `EnsureInitialized(force)`, `IsInitialized()` | `bool` |
| Read | `ReadChannel(ch)`, `ReadAllChannels()`, `RawManualFrames(ch, out, n)` | `ReadResult`, `ChannelReadings`, frame count |
| Modes | `EnterManualMode(ch)`, `EnterAuto1Mode(reset)`, `EnterAuto2Mode(reset)`, `GetMode()` | `bool` |
| Sequences | `ProgramAuto1Channels(mask)`, `ProgramAuto2LastChannel(ch)` | `bool` |
| Alarms and GPIOs | `ProgramAlarm()`, `ProgramAlarmVoltage()`, `ProgramGPIO(config)`, `SetGPIOOutputs(bits)` | `bool` |
| Range and power | `SetRange(range)`, `SetPowerDown(pd)` | `bool` |
| Conversion | `CountToVoltage()`, `VoltageToCount()`, `SetVref()`, `SetVA()`, `GetActiveVref()` | volts, counts |

Reads return a struct with an `error` field and an `ok()` helper (`Error::SpiError` when the port's `transfer()`
reported a bus error). The other calls return `false` for an argument out of range (a channel above 11) or a
failed transfer; see [Porting](porting.md#bus-errors).

## Initialise

```cpp
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

`EnsureInitialized()` discards the invalid first conversion, programs the Auto-1 mask and the Auto-2 last channel
from `ADS7952_CFG` and enters Auto-1. A second call returns `true` without SPI traffic; pass `force = true` after
the chip was reset or lost power. Every read returns `Error::NotInitialized` until it has succeeded.

## Configure

```cpp
--8<-- "tests/docs/api_tasks.cpp:configure"
```

Build masks with `ads7952::ChannelMask(0, 2, 4)`, `ChannelRangeMask(first, last)` or the constants
`kAllChannels`, `kEvenChannels`, `kOddChannels`, `kFirstFour`, `kSecondFour`, `kThirdFour`. Bit N is channel N.

`SetRange()` is in effect on the chip when it returns, in whatever mode the chip is in, and the driver's full scale
changes with it. [Modes and the frame pipeline](modes-and-pipeline.md#range-power-down-and-gpio-outputs) shows the
two frames it sends.

## Read

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

| Call | Frames (healthy) | Frames (most) | Leaves the chip in |
| --- | --- | --- | --- |
| `ReadChannel(ch)` | 3 | 1 + `MANUAL_READ_MAX_FRAMES` | Manual, on `ch` |
| `ReadAllChannels()` | N + 2 | 1 + N + `READ_ALL_MAX_EXTRA_FRAMES` | Auto-1 |

N is the number of channels in the Auto-1 mask.

`ReadAllChannels()` always uses the Auto-1 mask. `ChannelReadings::valid_mask` says which entries of `count[]` and
`voltage[]` were filled; on `Error::Timeout` the channels that did arrive are still valid.

## Program alarms and GPIOs

```cpp
--8<-- "tests/docs/api_tasks.cpp:alarms"
```

Each channel has a high and a low threshold. The chip compares the top 10 bits of each conversion with them, so a
threshold has a resolution of 4 counts. `ProgramAlarmVoltage()` converts with the reference that is active when you
call it: program the thresholds again after `SetRange()` or `SetVref()`.

| `GPIO01AlarmMode` | DI06:04 | GPIO0 | GPIO1 |
| --- | --- | --- | --- |
| `GPIO` | `000` | GPIO | GPIO |
| `GPIO0_HighAndLowAlarm` | `001` | High or low alarm | GPIO |
| `GPIO0_HighAlarm` | `010` | High alarm | GPIO |
| `GPIO1_LowAlarm` | `100` | GPIO | Low alarm |
| `GPIO1_LowAlarm_GPIO0_HighAlarm` | `110` | High alarm | Low alarm |

The chip has no code that makes GPIO1 a high-alarm output (SLAS605C Table 11). `GPIO1_HighAlarm`, the old, misleading
name of `GPIO1_LowAlarm`, is kept as a deprecated alias with the same code.

`GPIOConfig::gpio2_as_range_input` and `gpio3_as_powerdown_input` hand the range and power-down control to those
pins; `reset_all_registers` resets every chip register in the next frame. On the VQFN package only GPIO0 exists.

## Write outputs and power down

```cpp
--8<-- "tests/docs/api_tasks.cpp:write"
```

`SetGPIOOutputs()` and `SetPowerDown()` work like `SetRange()`: when they return, the pins show the new levels, or
the chip is powered down (about 1 µA) or awake again. Waking takes 1 µs and costs one invalid conversion, which
falls in the frame the driver drops.

## Read raw frames

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

`RawManualFrames()` sends one Manual frame and `n - 1` continue frames and returns every reply untouched. Use it
on the bench to see the address pipeline advance; it leaves the chip in Manual mode.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Signal | Meaning | What to do |
| --- | --- | --- |
| `Error::NotInitialized` | `EnsureInitialized()` has not succeeded | Call it |
| `Error::InvalidChannel` | Channel above 11 | Fix the caller |
| `Error::Timeout` | The requested channel address never came back in the frame budget | Check SPI mode, CS framing and MISO; re-initialise after a chip reset |
| `Error::SpiError` | The port's `transfer()` returned `false` | Check the port and the bus; the read stopped at that frame |
| A `bool` call returns `false` | Channel or last channel above 11, or the port's `transfer()` returned `false` | Fix the caller, or check the bus |

`ProgrammingFailed` and `ModeChangeFailed` exist in the enum but no call returns them today.
[Troubleshooting](troubleshooting.md) maps symptoms to causes.

## Calibrate the conversion

```cpp
--8<-- "tests/docs/api_tasks.cpp:calibrate"
```

`SetVref()` and `SetVA()` only change the numbers the driver converts with; they send nothing. Measure REFP once on
your board and pass the real value for the best absolute accuracy. [Configuration](configuration.md) covers the
limits these values are clamped to.
