---
layout: default
title: "🔩 CMake Integration"
description: "CMake integration guide for the HF-ADS7952 driver"
parent: "📚 Documentation"
nav_order: 6
permalink: /docs/cmake_integration/
---

# 🔩 CMake Integration

This guide covers the CMake build system integration for the HF-ADS7952 driver.

---

## Build System Overview

The library uses Modern CMake (3.16+) with the following structure:

```
hf-ads7952-driver/
├── CMakeLists.txt                           ← Root build file
├── cmake/
│   ├── hf_ads7952_build_settings.cmake      ← Version & settings (single source of truth)
│   └── hf_ads7952Config.cmake.in            ← find_package() template
├── inc/
│   ├── ads7952.hpp                          ← Main driver header
│   ├── ads7952.ipp                          ← Template implementation (included by .hpp)
│   ├── ads7952_registers.hpp                ← Register definitions
│   ├── ads7952_types.hpp                    ← Enums and structs
│   ├── ads7952_config.hpp                   ← Compile-time configuration
│   ├── ads7952_spi_interface.hpp            ← CRTP SPI base class
│   └── ads7952_version.h.in                 ← Version header template
└── src/                                     ← Empty (header-only library)
```

Since the driver is fully template-based (CRTP with `.ipp` implementation), it is built as a **CMake INTERFACE library** — no compiled object files.

---

## Linking the Library

### As a Subdirectory (Recommended)

```cmake
add_subdirectory(path/to/hf-ads7952-driver)
target_link_libraries(your_target PRIVATE hf::ads7952)
```

### Via find_package

After installing the library:

```cmake
find_package(hf_ads7952 REQUIRED)
target_link_libraries(your_target PRIVATE hf::ads7952)
```

### Via FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    hf_ads7952
    GIT_REPOSITORY https://github.com/hardfoc/hf-ads7952-driver.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(hf_ads7952)
target_link_libraries(your_target PRIVATE hf::ads7952)
```

---

## Build Settings File

The `cmake/hf_ads7952_build_settings.cmake` file is the **single source of truth** for all build variables:

| Variable | Purpose |
|----------|---------|
| `HF_ADS7952_VERSION` | Library version string (e.g., `1.0.0`) |
| `HF_ADS7952_VERSION_MAJOR/MINOR/PATCH` | Individual version components |
| `HF_ADS7952_TARGET_NAME` | CMake target name (`hf_ads7952`) |
| `HF_ADS7952_PUBLIC_INCLUDE_DIRS` | Include directories (source `inc/` + generated version header) |
| `HF_ADS7952_SOURCE_FILES` | Source files list (empty for header-only) |
| `HF_ADS7952_IDF_REQUIRES` | ESP-IDF component dependencies (`driver`) |

This file is included by both the root `CMakeLists.txt` and ESP-IDF component wrappers, ensuring consistency.

---

## Library Type Selection

The root `CMakeLists.txt` automatically selects:

| Condition | Library Type | Visibility |
|-----------|-------------|------------|
| `HF_ADS7952_SOURCE_FILES` empty | **INTERFACE** | Headers propagated to consumers |
| `HF_ADS7952_SOURCE_FILES` non-empty | **STATIC** | Compiled + headers propagated |

The ADS7952 driver is **INTERFACE** (header-only) because all code lives in `.hpp` and `.ipp` template files.

---

## Version Header Generation

The version is automatically generated from the template `inc/ads7952_version.h.in`:

```c
// Generated into build directory — do not edit
#define ADS7952_VERSION_MAJOR @HF_ADS7952_VERSION_MAJOR@
#define ADS7952_VERSION_MINOR @HF_ADS7952_VERSION_MINOR@
#define ADS7952_VERSION_PATCH @HF_ADS7952_VERSION_PATCH@
#define ADS7952_VERSION_STRING "@HF_ADS7952_VERSION@"
```

The generated header is placed in `${CMAKE_CURRENT_BINARY_DIR}/hf_ads7952_generated/` to keep the source tree clean. Both include paths (source `inc/` and generated directory) are automatically added to the target.

---

## ESP-IDF Component Integration

For ESP-IDF projects, the component wrapper is located at `examples/esp32/components/hf_ads7952/CMakeLists.txt`:

```cmake
include(${COMPONENT_DIR}/../../cmake/hf_ads7952_build_settings.cmake)

idf_component_register(
    SRCS ${HF_ADS7952_SOURCE_FILES}
    INCLUDE_DIRS ${HF_ADS7952_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_ADS7952_IDF_REQUIRES}
)
```

The `HF_ADS7952_IDF_REQUIRES` variable is set to `driver`, which pulls in ESP-IDF's SPI master driver.

---

## Optional Build Options

| CMake Option | Default | Description |
|-------------|---------|-------------|
| `HF_ADS7952_ENABLE_WARNINGS` | `OFF` | Enables `-Wall -Wextra -Wpedantic` for the library target |

```cmake
# Enable extra warnings during development
set(HF_ADS7952_ENABLE_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(path/to/hf-ads7952-driver)
```

---

## Compile-Time Configuration

The driver supports compile-time `#define` overrides (see [Configuration](configuration.md)):

```cmake
target_compile_definitions(your_target PRIVATE
    ADS7952_CFG_NUM_CHANNELS=8
    ADS7952_CFG_DEFAULT_RANGE=1
)
```

These values are used by `ads7952_config.hpp` to customize driver behavior without modifying library source.

---

## Compiler Requirements

| Requirement | Minimum |
|------------|---------|
| C++ Standard | C++20 |
| GCC | 10+ |
| Clang | 10+ |
| MSVC | 19.29+ |
| ESP-IDF | v5.x (ships GCC 13) |

The library enforces C++20 via:

```cmake
target_compile_features(hf_ads7952 INTERFACE cxx_std_20)
```

---

**Navigation**
⬅️ [Configuration](configuration.md)  ➡️ [API Reference](api_reference.md)
