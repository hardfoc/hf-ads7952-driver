---
layout: default
title: "🛠️ Installation"
description: "How to install and integrate the HF-ADS7952 driver library"
parent: "📚 Documentation"
nav_order: 1
permalink: /docs/installation/
---

# 🛠️ Installation

This guide explains how to obtain and install the HF-ADS7952 driver library.

---

## Requirements

| Tool           | Minimum Version | Notes |
| -------------- | --------------- | ----- |
| **CMake**      | 3.16+           | Build system |
| **Compiler**   | C++20 support   | GCC 10+, Clang 10+, MSVC 19.29+ |
| **ESP-IDF**    | 5.4+ (optional) | Required for ESP32 examples |

> 💡 The library is **header-only** (INTERFACE library). No separate compilation step is needed — just add it to your include path and link the CMake target.

---

## Option 1 — Git Submodule (Recommended)

Add as a submodule to your project:

```bash
git submodule add https://github.com/hardfoc/hf-ads7952-driver.git external/hf-ads7952-driver
git submodule update --init --recursive
```

Then in your `CMakeLists.txt`:

```cmake
add_subdirectory(external/hf-ads7952-driver)
target_link_libraries(your_target PRIVATE hf::ads7952)
```

---

## Option 2 — CMake FetchContent

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

## Option 3 — System Install

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

Then in a consuming project:

```cmake
find_package(hf_ads7952 REQUIRED)
target_link_libraries(your_target PRIVATE hf::ads7952)
```

---

## ESP-IDF Component

For ESP-IDF projects, use the component wrapper in `examples/esp32/components/hf_ads7952/`:

```
your_project/
├── main/
│   └── app_main.cpp
└── components/
    └── hf_ads7952/
        ├── CMakeLists.txt       ← Wraps the library for idf_component_register()
        └── idf_component.yml    ← Component manifest
```

The component `CMakeLists.txt` includes `hf_ads7952_build_settings.cmake` and registers all headers with the ESP-IDF build system. See [CMake Integration](cmake_integration.md) for the full details.

---

## Library Structure

```
hf-ads7952-driver/
├── CMakeLists.txt                     ← Main build file (INTERFACE library)
├── README.md
├── cmake/
│   ├── hf_ads7952_build_settings.cmake   ← Version & file lists (single source of truth)
│   └── hf_ads7952Config.cmake.in         ← find_package() template
├── inc/
│   ├── ads7952.hpp                    ← Main driver header (include this)
│   ├── ads7952_config.hpp             ← Compile-time configuration
│   ├── ads7952_registers.hpp          ← SPI frame & register constants
│   ├── ads7952_spi_interface.hpp      ← CRTP SPI abstraction
│   ├── ads7952_types.hpp              ← Enums, structs, error codes
│   └── ads7952_version.h.in           ← Version template
├── src/
│   └── ads7952.ipp                    ← Template method implementations
├── docs/                              ← Documentation
└── examples/
    └── esp32/                         ← ESP-IDF example projects
```

---

## Verify Installation

After installation, verify the driver is accessible:

```cpp
#include "ads7952.hpp"

// Check version at compile time
static_assert(ads7952::ADS7952<MySpi>::GetDriverVersionMajor() >= 1,
              "ADS7952 driver v1+ required");
```

Or print at runtime:

```cpp
printf("ADS7952 driver v%u.%u.%u\n",
       ads7952::ADS7952<MySpi>::GetDriverVersionMajor(),
       ads7952::ADS7952<MySpi>::GetDriverVersionMinor(),
       ads7952::ADS7952<MySpi>::GetDriverVersionPatch());
```

---

**Navigation**
⬅️ [Documentation Home](index.md)  ➡️ [Quick Start](quickstart.md)
