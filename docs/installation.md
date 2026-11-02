# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++20 library with no dependencies.

## Requirements

- A C++20 compiler: GCC 10+, Clang 12+ or a current MSVC.
- No RTOS, heap or exceptions: every driver call is `noexcept` and returns `bool` or a result struct.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-ads7952-driver.git external/hf-ads7952-driver
git submodule update --init external/hf-ads7952-driver
```

Only `inc/`, `src/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-ads7952-driver)
target_link_libraries(my_app PRIVATE hf::ads7952)   # or hf_ads7952
```

The `hf_ads7952` target (alias `hf::ads7952`) is an `INTERFACE` library. It carries the include directories
(`inc/`, `src/` and the generated version header) and requires C++20 (`cxx_std_20`), so your target does not need
to set either.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_ADS7952_ENABLE_WARNINGS` | `OFF` | Adds `-Wall -Wextra -Wpedantic` to targets that link the driver |
| `HF_ADS7952_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

`cmake --install` installs the headers and a package configuration, so an installed copy is found with
`find_package(hf_ads7952 CONFIG REQUIRED)` and linked as `hf::ads7952`.

## ESP-IDF

The ESP-IDF component wrapper is
[`examples/esp32/components/hf_ads7952/`](../examples/esp32/components/hf_ads7952/CMakeLists.txt). Copy that folder
into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the `HF_ADS7952_ROOT` cache
variable to the driver checkout; the wrapper stops with a clear message when the path is wrong. Your component then
declares `REQUIRES hf_ads7952`.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_ads7952_build_settings.cmake`](../cmake/hf_ads7952_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | `INTERFACE` target, install and package export |
| ESP-IDF | [`examples/esp32/components/hf_ads7952/CMakeLists.txt`](../examples/esp32/components/hf_ads7952/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `ads7952_version.h` at configure time
(`HF_ADS7952_VERSION_MAJOR`, `_MINOR`, `_PATCH`, `HF_ADS7952_VERSION_STRING`). The driver class also reports it:
`ADS7952<MySpi>::GetDriverVersionMajor()`, `GetDriverVersionMinor()` and `GetDriverVersionPatch()`.

## Compile-time switches

The defaults in [`inc/ads7952_config.hpp`](../inc/ads7952_config.hpp) can be overridden with `CONFIG_ADS7952_*`
defines: reference and supply voltage, the power-up Auto-1 mask and Auto-2 last channel, the default range and the
frame budgets of the read loops. [Configuration](configuration.md#compile-time-defaults) lists them.

## Next

[Quickstart](quickstart.md) wires the chip and reads every channel; [Porting](porting.md) writes the SPI interface
for your MCU.
