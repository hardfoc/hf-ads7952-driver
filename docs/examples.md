# Examples

This page walks through all 4 example applications included in the repository. Examples are located in `examples/esp32/main/` and target the ESP32-S3 with ESP-IDF.

---

## Example Overview

| App Name | Source File | Difficulty | Description |
|----------|-----------|------------|-------------|
| **Basic ADC Reading** | `basic_adc_reading_example.cpp` | ⭐ Beginner | Initialize, manual reads, batch reads |
| **Multi-Mode** | `multi_mode_example.cpp` | ⭐⭐ Intermediate | Manual, Auto-1, Auto-2 modes, channel masks, range |
| **Alarm & GPIO** | `alarm_gpio_example.cpp` | ⭐⭐⭐ Advanced | GPIO outputs, alarm thresholds, alarm routing |
| **Integration Tests** | `driver_integration_test.cpp` | ⭐⭐⭐ Advanced | 30+ test cases across 9 sections |

---

## 1. Basic ADC Reading

**Source:** `examples/esp32/main/basic_adc_reading_example.cpp`

This example demonstrates the fundamental operations: initialization, single-channel reads (Manual mode), and batch reads (Auto-1 mode).

### Key Concepts

```cpp
// Create and initialize
ads7952::ADS7952<Esp32Ads7952SpiBus> adc(*bus, 2.5f, 5.0f);
adc.EnsureInitialized();  // Idempotent — discards first conversion, programs defaults

// Manual mode: read each channel individually
for (uint8_t ch = 0; ch < 12; ++ch) {
    auto result = adc.ReadChannel(ch);
    if (result.ok()) {
        printf("CH%02u: %4u counts  (%.3f V)\n", ch, result.count, result.voltage);
    }
}

// Auto-1 mode: batch read all channels
adc.EnterAuto1Mode(true);
auto readings = adc.ReadAllChannels();
if (readings.ok()) {
    for (uint8_t ch = 0; ch < 12; ++ch) {
        if (readings.hasChannel(ch))
            printf("CH%02u: %.3f V\n", ch, readings.voltage[ch]);
    }
}
```

### What to Look For

- `ReadResult.ok()` — always check before using the data
- `ChannelReadings.hasChannel(ch)` — verifies the channel was in the Auto-1 mask
- The driver version is printed at startup via `GetDriverVersionMajor/Minor/Patch()`

---

## 2. Multi-Mode Example

**Source:** `examples/esp32/main/multi_mode_example.cpp`

Demonstrates all three operating modes and programmable channel sequencing.

### Sections

1. **Manual Mode** — reads specific channels (CH0, CH5, CH11)
2. **Auto-1 Full Scan** — batch reads all 12 channels
3. **Auto-1 Subset** — reprograms to even channels only (`kEvenChannels`)
4. **Auto-2 Mode** — sequential scan CH0–CH5
5. **Range Comparison** — reads CH0 at both Vref and 2×Vref, showing the voltage difference

### Key Code

```cpp
// Auto-1 subset: only even channels
adc.ProgramAuto1Channels(ads7952::kEvenChannels);  // CH0, CH2, CH4, CH6, CH8, CH10
adc.EnterAuto1Mode(true);
auto even = adc.ReadAllChannels();

// Auto-2: sequential scan CH0 through CH5
adc.ProgramAuto2LastChannel(5);
adc.EnterAuto2Mode(true);

// Range comparison on CH0
adc.SetRange(ads7952::Range::Vref);
auto r_vref = adc.ReadChannel(0);

adc.SetRange(ads7952::Range::TwoVref);
auto r_2vref = adc.ReadChannel(0);
// Same input signal → different count values due to different reference
```

---

## 3. Alarm & GPIO Example

**Source:** `examples/esp32/main/alarm_gpio_example.cpp`

Demonstrates the ADS7952's GPIO and alarm threshold features.

### Sections

1. **GPIO Configuration** — configures GPIO2 and GPIO3 as outputs
2. **GPIO Toggle** — alternately drives GPIO2/GPIO3 high/low
3. **Alarm Programming** — sets low (0.5 V) and high (2.0 V) alarm thresholds on CH0, and count-based thresholds on CH4
4. **Alarm-as-Output** — configures GPIO0 as combined hi/lo alarm output
5. **Monitoring Loop** — reads CH0 and CH4, compares against programmed thresholds

### Key Code

```cpp
// Configure GPIO pins as outputs
ads7952::GPIOConfig gpio_cfg{};
gpio_cfg.direction_mask = ads7952::gpio::kGPIO2 | ads7952::gpio::kGPIO3;
gpio_cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO;
adc.ProgramGPIO(gpio_cfg);

// Toggle outputs
adc.SetGPIOOutputs(ads7952::gpio::kGPIO2);  // GPIO2 = HIGH
adc.SetGPIOOutputs(ads7952::gpio::kGPIO3);  // GPIO3 = HIGH

// Voltage-based alarm API — pass voltage directly, no manual conversion
adc.ProgramAlarmVoltage(0, ads7952::AlarmBound::Low,  0.5f);  // CH0 low  alarm at 0.5 V
adc.ProgramAlarmVoltage(0, ads7952::AlarmBound::High, 2.0f);  // CH0 high alarm at 2.0 V

// Count-based alarm API — pass raw 12-bit ADC counts directly
adc.ProgramAlarm(4, ads7952::AlarmBound::Low,  500);   // CH4 low  alarm at count 500
adc.ProgramAlarm(4, ads7952::AlarmBound::High, 3500);  // CH4 high alarm at count 3500

// Configure GPIO0 as combined alarm output
ads7952::GPIOConfig alarm_gpio{};
alarm_gpio.alarm_mode = ads7952::GPIO01AlarmMode::GPIO0_HighAndLowAlarm;
alarm_gpio.direction_mask = ads7952::gpio::kGPIO2 | ads7952::gpio::kGPIO3;
adc.ProgramGPIO(alarm_gpio);

// Monitoring: compare using voltage for voltage-programmed channels,
// counts for count-programmed channels
auto r0 = adc.ReadChannel(0);
if (r0.voltage < 0.5f) /* low alarm */ ;
if (r0.voltage > 2.0f) /* high alarm */ ;

auto r4 = adc.ReadChannel(4);
if (r4.count < 500)  /* low alarm */ ;
if (r4.count > 3500) /* high alarm */ ;
```

---

## 4. Integration Test Suite

**Source:** `examples/esp32/main/driver_integration_test.cpp`

Comprehensive test suite with 30+ individual test cases across 9 sections. Uses a lightweight test framework with automatic pass/fail counting.

### Test Sections

| Section | Tests | Description |
|---------|-------|-------------|
| INITIALIZATION | 2 | SPI bus init, driver `EnsureInitialized()` |
| CHANNEL_READING | 4 | Single channel, all 12, invalid channel, Auto-1 batch |
| MODE_SWITCHING | 4 | Enter Manual/Auto-1/Auto-2, round-trip switching |
| AUTO_SEQUENCE | 3 | Auto-1 subset mask, Auto-2 last channel, invalid channel |
| RANGE | 3 | Set Vref, set 2×Vref, reading after range change |
| ALARM_GPIO | 6 | Low/high alarm (count), voltage alarm, invalid channel alarm, GPIO default, GPIO outputs |
| POWER_DOWN | 1 | Power down and resume |
| VOLTAGE_CONVERSION | 6 | Zero, max, midscale, instance vs static, `VoltageToCount` accuracy, boundary clamp |
| STABILITY | 2 | 100× repeated manual reads, 50× Auto-1 batch reads |

### Running Tests

```bash
cd examples/esp32
./scripts/build_app.sh driver_integration_test Debug
./scripts/flash_app.sh driver_integration_test Debug
```

Output is printed via ESP-IDF logging with pass/fail summary at the end.

---

## Building ESP32 Examples

### Using Build Scripts

The `examples/esp32/scripts/` directory (git submodule: `hf-espidf-project-tools`) provides build automation:

```bash
cd examples/esp32

# Build a specific app
./scripts/build_app.sh basic_adc_reading Debug

# List available apps
./scripts/build_app.sh list

# Flash and monitor
./scripts/flash_app.sh basic_adc_reading Debug
```

### Using idf.py Directly

```bash
cd examples/esp32

# Set target
idf.py set-target esp32s3

# Build with app selection
idf.py -B build \
    "-DAPP_TYPE=basic_adc_reading" \
    "-DBUILD_TYPE=Debug" \
    "-DAPP_SOURCE_FILE=basic_adc_reading_example.cpp" \
    build

# Flash and monitor
idf.py -B build flash monitor
```

### App Configuration

App definitions are centralized in `app_config.yml`:

```yaml
apps:
  basic_adc_reading:
    source_file: "basic_adc_reading_example.cpp"
    category: "example"

  driver_integration_test:
    source_file: "driver_integration_test.cpp"
    category: "test"

  multi_mode:
    source_file: "multi_mode_example.cpp"
    category: "example"

  alarm_gpio:
    source_file: "alarm_gpio_example.cpp"
    category: "example"
```

---

## Custom Platform Example

To use on a non-ESP32 platform:

1. Implement `SpiInterface<YourSpi>` (see [Platform Integration](platform_integration.md))
2. Include `ads7952.hpp`
3. Create, initialize, and use the driver:

```cpp
#include "ads7952.hpp"
#include "my_spi.hpp"

MySpi spi(/* config */);
ads7952::ADS7952<MySpi> adc(spi, 2.5f, 5.0f);

if (!adc.EnsureInitialized()) {
    printf("Init failed!\n");
    return;
}

auto r = adc.ReadChannel(0);
if (r.ok()) {
    printf("CH0: %.3f V\n", r.voltage);
}
```

---

**Navigation**
⬅️ [API Reference](api_reference.md)  ➡️ [Troubleshooting](troubleshooting.md)
