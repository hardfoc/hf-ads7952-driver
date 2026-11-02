# Testing and CI

How the driver is checked: host unit tests against a model of the chip, compiled documentation, ESP32 builds and
static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the ADS7952's SPI behaviour: a channel selected in frame N is
returned in frame N + 2 with its address in DO15:12, Auto-1 and Auto-2 walk their sequences, continue frames ignore
DI11:00, and the program registers (Auto-1 mask, Auto-2 last channel, GPIO, alarms) record what they receive.

```bash
cmake -S . -B build-host -DHF_ADS7952_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Initialisation, single and sequence reads, error codes, programming frames, range latching, conversion helpers, raw frames |
| [`tests/mock_ads7952.hpp`](../tests/mock_ads7952.hpp) | The chip model |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_spi.hpp), which the host build
compiles (target `hf_ads7952_doc_snippets`, warnings on) with the driver instantiated over the documented port. A
change to the API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every CI-enabled app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test suite: `driver_integration_test` runs every driver call
and prints a pass / fail summary over the serial monitor.
