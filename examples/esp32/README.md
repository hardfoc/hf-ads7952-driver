# ESP32 ADS7952 Driver Examples

ESP-IDF example applications for the ADS7952 12-channel, 12-bit SAR ADC driver targeting the **ESP32-S3**.

## Examples

| App Type | Source File | Category | Description |
| --- | --- | --- | --- |
| `basic_adc_reading` | `basic_adc_reading_example.cpp` | example | Initialize, manual single-channel reads, Auto-1 batch reads |
| `multi_mode` | `multi_mode_example.cpp` | example | Manual, Auto-1, Auto-2 modes, channel masks, range comparison |
| `alarm_gpio` | `alarm_gpio_example.cpp` | example | GPIO outputs, alarm thresholds, alarm-as-output routing |
| `driver_integration_test` | `driver_integration_test.cpp` | test | 30+ test cases across 9 sections |

## Building

### Using Build Scripts

```bash
cd examples/esp32

# List available apps
./scripts/build_app.sh list

# Build a specific app
./scripts/build_app.sh basic_adc_reading Debug

# Flash and monitor
./scripts/flash_app.sh basic_adc_reading Debug
```

### Using idf.py Directly

```bash
cd examples/esp32
idf.py set-target esp32s3
idf.py -B build \
    "-DAPP_TYPE=basic_adc_reading" \
    "-DBUILD_TYPE=Debug" \
    "-DAPP_SOURCE_FILE=basic_adc_reading_example.cpp" \
    build
idf.py -B build flash monitor
```

## Hardware Configuration

Edit `main/esp32_ads7952_test_config.hpp` to match your hardware:

| Setting | Default | Description |
| --- | --- | --- |
| `SPIPins::MISO` | 13 | SPI MISO (SDO from ADS7952) |
| `SPIPins::MOSI` | 11 | SPI MOSI (SDI to ADS7952) |
| `SPIPins::SCLK` | 12 | SPI SCLK |
| `SPIPins::CS` | 10 | SPI CS, driven by the SPI peripheral |
| `SPIParams::FREQUENCY` | 4 MHz | SPI clock frequency (ADS7952 maximum: 20 MHz) |
| `SPIParams::MODE` | 0 | SPI mode (CPOL = 0, CPHA = 0) |
| `ADCSpecs::VREF` | 2.5 V | External reference voltage |

## Directory Structure

```
examples/esp32/
├── CMakeLists.txt                          # Root project CMake
├── app_config.yml                          # App definitions for build scripts
├── sdkconfig                               # ESP-IDF project config
├── README.md                               # This file
├── components/
│   └── hf_ads7952/
│       ├── CMakeLists.txt                  # ESP-IDF component wrapper
│       └── idf_component.yml               # Component manifest
├── main/
│   ├── CMakeLists.txt                      # Main component CMake
│   ├── TestFramework.h                     # Lightweight test framework
│   ├── esp32_ads7952_bus.hpp               # ESP32 SPI CRTP implementation
│   ├── esp32_ads7952_test_config.hpp       # Hardware pin configuration
│   ├── basic_adc_reading_example.cpp       # Basic example
│   ├── multi_mode_example.cpp              # Multi-mode example
│   ├── alarm_gpio_example.cpp              # Alarm & GPIO example
│   └── driver_integration_test.cpp         # Integration tests
└── scripts/                                # Build scripts (git submodule)
```

## SPI Implementation

The ESP32 SPI interface is implemented in `esp32_ads7952_bus.hpp` using the CRTP pattern:

```cpp
class Esp32Ads7952SpiBus : public ads7952::SpiInterface<Esp32Ads7952SpiBus> {
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len);
};
```

Each `transfer()` call performs one 16-bit full-duplex SPI transaction with CS framing.
