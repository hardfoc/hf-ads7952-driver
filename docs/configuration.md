# Configuration

The values the driver needs about your board, where each one is set, and the compile-time defaults behind them.

## Run-time settings

| Setting | Set with | Default | Sent to the chip |
| --- | --- | --- | --- |
| Reference voltage (REFP) | constructor `vref`, `SetVref()` | `DEFAULT_VREF`, 2.5 V | No: conversion only |
| Analog supply (+VA) | constructor `va`, `SetVA()` | `DEFAULT_VA`, 5.0 V | No: stored for your use (`GetVA()`) |
| Input range | constructor `initial_range`, `SetRange()` | `DEFAULT_RANGE`, `Range::Vref` | Yes, with the next mode frame |
| Auto-1 mask | `ProgramAuto1Channels()` | `DEFAULT_AUTO1_MASK`, `0x0FFF` | Yes, by `EnsureInitialized()` |
| Auto-2 last channel | `ProgramAuto2LastChannel()` | `DEFAULT_AUTO2_LAST_CH`, 11 | Yes, by `EnsureInitialized()` |
| Power-down | `SetPowerDown()` | `PowerDown::Normal` | Yes, with the next mode frame |
| GPIO functions and outputs | `ProgramGPIO()`, `SetGPIOOutputs()` | all inputs | Yes |

The constructor's `initial_range` sets the driver's full scale only. The chip starts in the 0 to VREF range after
power-up and takes `Range::TwoVref` with the first mode frame, which `EnsureInitialized()` sends.

```cpp
--8<-- "tests/docs/api_tasks.cpp:configure"
```

## Reference and range

The active reference is VREF in `Range::Vref` and 2 × VREF in `Range::TwoVref`. A count converts as
`count × active reference / 4095`.

| Range | Measurable span | Condition (datasheet) |
| --- | --- | --- |
| `Range::Vref` | 0 V to VREF | VREF 2.0 V to 3.0 V |
| `Range::TwoVref` | 0 V to 2 × VREF | 2 × VREF ≤ +VA |

!!! note "Clamping of `vref` and `va`"
    The constructor, `SetVref()` and `SetVA()` clamp their arguments to `ADS7952_CFG::MIN_VREF` ...
    `MAX_VREF` (1.0 V to 2.5 V) and `MIN_VA` ... `MAX_VA` (2.7 V to 5.5 V). The datasheet's reference range is
    2.0 V to 3.0 V and its +VA maximum is 5.25 V, so a reference above 2.5 V is converted as 2.5 V. Use a 2.5 V
    reference, or change `MAX_VREF`, until the limits are aligned.

The driver does not limit `Range::TwoVref` to +VA: with 2.5 V at REFP, use it only on a 5 V analog supply.

## Compile-time defaults

[`inc/ads7952_config.hpp`](../inc/ads7952_config.hpp) defines the defaults in the `ADS7952_CFG` namespace. Each
can be overridden with a define, from Kconfig in ESP-IDF or from CMake:

| Define | Constant | Default | Effect |
| --- | --- | --- | --- |
| `CONFIG_ADS7952_VREF_MV` | `DEFAULT_VREF` | 2500 | Constructor default for `vref`, in mV |
| `CONFIG_ADS7952_VA_MV` | `DEFAULT_VA` | 5000 | Constructor default for `va`, in mV |
| `CONFIG_ADS7952_RANGE_2VREF` | `DEFAULT_RANGE` | not set (`Range::Vref`) | Defined: `Range::TwoVref` |
| `CONFIG_ADS7952_AUTO1_CHANNEL_MASK` | `DEFAULT_AUTO1_MASK` | `0x0FFF` | Mask `EnsureInitialized()` programs |
| `CONFIG_ADS7952_AUTO2_LAST_CH` | `DEFAULT_AUTO2_LAST_CH` | 11 | Last channel `EnsureInitialized()` programs |
| `CONFIG_ADS7952_MANUAL_READ_MAX_FRAMES` | `MANUAL_READ_MAX_FRAMES` | 4 | Continue frames `ReadChannel()` tries |
| `CONFIG_ADS7952_READ_ALL_MAX_EXTRA` | `READ_ALL_MAX_EXTRA_FRAMES` | 4 | Spare frames for `ReadAllChannels()` |
| `CONFIG_ADS7952_MODE_AUTO1`, `_AUTO2` | `DEFAULT_MODE` | Manual | Defined but not used by the driver |
| `CONFIG_ADS7952_MAX_RETRIES` | `MAX_RETRIES` | 3 | Defined but not used by the driver |

```cmake
target_compile_definitions(my_app PRIVATE
    CONFIG_ADS7952_VREF_MV=2048
    CONFIG_ADS7952_AUTO1_CHANNEL_MASK=0x000F)   # CH0..CH3
```

The defines must be visible wherever `ads7952.hpp` is included; set them on the target, not in one source file.
`ESP-IDF` has no Kconfig file for the driver: add the options to your project's `Kconfig` with these names and
`sdkconfig.h` provides the `CONFIG_` macros.

## Where the defaults come from

Each value is in [`inc/ads7952_config.hpp`](../inc/ads7952_config.hpp) and in the
[API reference](api-reference.md); the chip's own reset state (Manual mode, CH0, range 1, all GPIOs inputs) is in
the [datasheet](datasheet/ads7952.pdf), Tables 1 to 13.
