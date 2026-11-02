# Configuration

The ADS7952 driver provides compile-time configuration through `ads7952_config.hpp`. All defaults can be overridden via Kconfig (ESP-IDF), CMake defines, or direct preprocessor definitions.

---

## Default Configuration

All defaults are defined in the `ADS7952_CFG` namespace:

| Option | Default | Description |
|--------|---------|-------------|
| `DEFAULT_MODE` | `Mode::Manual` | Initial operating mode |
| `DEFAULT_RANGE` | `Range::Vref` | Default input range |
| `DEFAULT_VREF` | `2.5f` V | Reference voltage (REFP pin) |
| `DEFAULT_VA` | `5.0f` V | Analog supply voltage |
| `DEFAULT_AUTO1_MASK` | `0x0FFF` | Auto-1 channel mask (all 12 channels) |
| `DEFAULT_AUTO2_LAST_CH` | `11` | Auto-2 last channel |
| `NUM_CHANNELS` | `12` | Number of analog input channels |
| `RESOLUTION_BITS` | `12` | ADC resolution |
| `MAX_COUNT` | `4095` | Maximum ADC count (2¹² − 1) |
| `READ_ALL_MAX_EXTRA_FRAMES` | `4` | Extra SPI frames budget for `ReadAllChannels()` |
| `MAX_RETRIES` | `3` | Max retries for mode change / programming |

---

## Kconfig Integration (ESP-IDF)

When building under ESP-IDF, the driver uses Kconfig preprocessor defines to override defaults at build time. Define these in your project's `Kconfig` menu:

```kconfig
menu "ADS7952 Configuration"
    config ADS7952_MODE_AUTO1
        bool "Default to Auto-1 mode"
        default n
        help
            If enabled, the driver defaults to Auto-1 mode instead of Manual.

    config ADS7952_MODE_AUTO2
        bool "Default to Auto-2 mode"
        default n

    config ADS7952_RANGE_2VREF
        bool "Default to 2×Vref range"
        default n

    config ADS7952_VREF_MV
        int "Reference voltage in millivolts"
        default 2500
        help
            REFP pin voltage in mV. Default 2500 for REF5025.

    config ADS7952_VA_MV
        int "Analog supply voltage in millivolts"
        default 5000

    config ADS7952_AUTO1_CHANNEL_MASK
        hex "Auto-1 channel bitmask"
        default 0x0FFF
        help
            Bitmask of channels for Auto-1 sequencing. Bit N enables CH N.

    config ADS7952_AUTO2_LAST_CH
        int "Auto-2 last channel (0-11)"
        default 11

    config ADS7952_READ_ALL_MAX_EXTRA
        int "Extra frames for ReadAllChannels"
        default 4

    config ADS7952_MAX_RETRIES
        int "Max retries for programming operations"
        default 3
endmenu
```

The `ads7952_config.hpp` header checks for each `CONFIG_*` define:

```cpp
#ifdef CONFIG_ADS7952_VREF_MV
inline constexpr float DEFAULT_VREF = CONFIG_ADS7952_VREF_MV / 1000.0f;
#else
inline constexpr float DEFAULT_VREF = 2.5f;
#endif
```

### CMake Defines (Non-ESP-IDF)

For non-ESP-IDF projects, pass defines via CMake:

```cmake
target_compile_definitions(your_target PRIVATE
    CONFIG_ADS7952_VREF_MV=2500
    CONFIG_ADS7952_VA_MV=5000
    CONFIG_ADS7952_AUTO1_CHANNEL_MASK=0x000F  # CH0-CH3 only
)
```

---

## Operating Modes

The ADS7952 has three operating modes, controlled via `EnterManualMode()`, `EnterAuto1Mode()`, and `EnterAuto2Mode()`:

### Manual Mode

- **You select the channel** in each call to `ReadChannel(ch)`
- The driver sends a 2-frame SPI sequence: MANUAL + channel select → CONTINUE → read response
- Best for: reading specific channels on-demand, targeted measurements

```cpp
auto r = adc.ReadChannel(5);  // Reads CH5 specifically
```

### Auto-1 Mode

- The ADC **automatically sequences** through a programmable bitmask of channels
- Program with `ProgramAuto1Channels(mask)` — bit N enables CH N (bit 0 = CH0, LSB-first)
- Use `ads7952::ChannelMask()`, `ads7952::ChannelRangeMask()`, or predefined constants
- `ReadAllChannels()` uses Auto-1 mode with counter reset to batch-read all enabled channels
- Best for: periodic multi-channel scanning, motor control sampling loops

```cpp
adc.ProgramAuto1Channels(ads7952::kFirstFour);             // CH0–CH3
adc.ProgramAuto1Channels(ads7952::ChannelMask(0, 5, 11));  // Arbitrary
adc.EnterAuto1Mode(true);           // Reset counter to start
auto readings = adc.ReadAllChannels();
```

### Auto-2 Mode

- The ADC **sequentially scans** from CH0 through a programmable last channel
- Program with `ProgramAuto2LastChannel(ch)`
- Best for: contiguous channel ranges (e.g., CH0–CH5)

```cpp
adc.ProgramAuto2LastChannel(5);  // Scan CH0 through CH5
adc.EnterAuto2Mode(true);
```

### Mode Comparison

| Feature | Manual | Auto-1 | Auto-2 |
|---------|--------|--------|--------|
| Channel selection | Per-read | Bitmask | 0 to N |
| Programmable subset | N/A | ✅ Any combination | ✅ Contiguous only |
| Best for | Single-channel reads | Arbitrary multi-channel | Contiguous sweeps |
| `ReadAllChannels()` | ❌ | ✅ Uses this internally | ❌ |

---

## Input Range

| Setting | Range enum | Input Span | Active Reference | Bit |
|---------|-----------|------------|-----------------|-----|
| **Default** | `Range::Vref` | 0 V to Vref | `vref_` | 0 |
| **Extended** | `Range::TwoVref` | 0 V to 2×Vref | `min(2×vref, va)` | 1 |

```cpp
adc.SetRange(ads7952::Range::TwoVref);  // Doubles the input span
float active_ref = adc.GetActiveVref();  // Check what's being used
```

> 💡 When `TwoVref` is selected, the driver clamps the active reference to `min(2 × vref, va)` so voltage conversions never exceed the analog supply.

---

## Power-Down Control

| Setting | Description |
|---------|-------------|
| `PowerDown::Normal` | Full power, normal ADC operation |
| `PowerDown::PowerDown` | Device enters power-down mode. First conversion after wake-up may be invalid. |

```cpp
adc.SetPowerDown(ads7952::PowerDown::PowerDown);  // Enter low-power state
// ... later ...
adc.SetPowerDown(ads7952::PowerDown::Normal);      // Resume operation
```

The power-down bit is included in every SPI frame via `commonControlBits()`, so it takes effect on the next SPI transfer.

---

## Voltage Reference Configuration

The constructor accepts two voltage parameters that directly affect ADC accuracy:

| Parameter | Constructor Arg | Description |
|-----------|----------------|-------------|
| **Vref** | `vref` (default 2.5) | External reference voltage on REFP pin |
| **VA** | `va` (default 5.0) | Analog supply voltage |

```cpp
// Custom reference: 3.0V reference, 3.3V supply
ads7952::ADS7952<MySpi> adc(spi, 3.0f, 3.3f);
```

The driver computes `two_vref_ = min(2 × vref, va)` at construction time, ensuring `CountToVoltage()` always returns accurate results regardless of range setting.

---

## Auto-1 Channel Mask Examples

| Mask | Constant | Enabled Channels | Description |
|------|----------|-------------------|-------------|
| `0x0FFF` | `kAllChannels` | CH0–CH11 | All 12 channels (default) |
| `0x000F` | `kFirstFour` | CH0–CH3 | First 4 channels |
| `0x0555` | `kEvenChannels` | CH0, CH2, CH4, CH6, CH8, CH10 | Even channels only |
| `0x0AAA` | `kOddChannels` | CH1, CH3, CH5, CH7, CH9, CH11 | Odd channels only |
| `0x0003` | — | CH0, CH1 | Two-channel differential pair |
| `0x0001` | — | CH0 | Single channel (high-speed repeated sampling) |

> 💡 **Bit ordering:** bit N = channel N, bit 0 = CH0 (LSB). Use `ads7952::ChannelMask(ch1, ch2, ...)` for arbitrary sets.

---

**Navigation**
⬅️ [Platform Integration](platform_integration.md)  ➡️ [CMake Integration](cmake_integration.md)
