# Examples

Complete ESP32 applications that exercise the driver on real hardware, one feature area each.

They run on an ESP32-S3 wired to an ADS7952; the pins are in
[`esp32_ads7952_test_config.hpp`](../examples/esp32/main/esp32_ads7952_test_config.hpp) and the port in
[`esp32_ads7952_bus.hpp`](../examples/esp32/main/esp32_ads7952_bus.hpp). CI builds every app marked "yes" below on
each pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `driver_integration_test` :material-star:{ title="featured" } | Comprehensive driver integration test suite (hardware required for full validation) | [`driver_integration_test.cpp`](../examples/esp32/main/driver_integration_test.cpp) | yes |
| `basic_adc_reading` :material-star:{ title="featured" } | Basic ADC channel reading example with voltage conversion | [`basic_adc_reading_example.cpp`](../examples/esp32/main/basic_adc_reading_example.cpp) | no |
| `multi_mode` :material-star:{ title="featured" } | Demonstrates Manual, Auto-1, and Auto-2 operating modes | [`multi_mode_example.cpp`](../examples/esp32/main/multi_mode_example.cpp) | no |
| `alarm_gpio` :material-star:{ title="featured" } | Alarm threshold programming and GPIO pin control | [`alarm_gpio_example.cpp`](../examples/esp32/main/alarm_gpio_example.cpp) | no |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh driver_integration_test Release
./scripts/flash_app.sh flash_monitor driver_integration_test Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes from)
and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See initialisation, a single read and a full scan | `basic_adc_reading_example.cpp` |
| Compare Manual, Auto-1 and Auto-2, and the two ranges | `multi_mode_example.cpp` |
| Program alarm windows and drive the GPIOs | `alarm_gpio_example.cpp` |
| Run every driver call with pass / fail checks | `driver_integration_test.cpp` |

The [example folder's README](../examples/esp32/README.md) has the build commands, the `idf.py` invocation and the pin table.
