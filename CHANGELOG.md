# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses semantic versioning.

## [Unreleased]

The driver has not been released yet; its version in `cmake/hf_ads7952_build_settings.cmake` is 1.0.0.

### Added

- Host unit tests (`-DHF_ADS7952_BUILD_TESTS=ON`) against a model of the chip's SPI pipeline, and a CI workflow.

### Changed

- The SPI port's `transfer()` now returns `bool` (`true` = frame clocked, `false` = bus error), and the driver
  propagates it: `bool` calls return `false`, `ReadChannel()` / `ReadAllChannels()` return `Error::SpiError`,
  `RawManualFrames()` returns the number of frames that got through, and the driver's mirrored state only changes
  after a successful transfer. `SetGPIOOutputs()` returns `bool` instead of `void`. Ports with a `void` `transfer()`
  still compile (detected at compile time) and are treated as never failing; any other return type is a compile
  error, so an SDK status code is not silently read as a bool. The ESP32 example port returns `false` on an
  ESP-IDF error.
- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, modes and the frame
  pipeline, architecture, API guide, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files are gone.

### Fixed

- `SetRange()`, `SetPowerDown()` and `SetGPIOOutputs()` take effect on the chip before they return. They sent a
  continue frame, whose DI11:00 the chip ignores (SLAS605C Table 7), so the change only landed at the next
  `Enter...Mode()` while `CountToVoltage()` had already switched scale: voltages were off by a factor of 2 in
  between. They now send a mode frame for the current mode with the programming bit DI11 = 1 (Tables 1, 2, 5),
  then one flush frame, after which the range applies to every result clocked out (Figure 50), the GPIO pins show
  the new levels (section 8.1) and the chip is powered down or awake (section 8.3.2). The driver's range, active
  reference, power-down and GPIO state change only once the chip has latched them. In Auto-1 / Auto-2 the two
  frames advance the channel counter by two. New getters: `GetPowerDown()`, `GetGPIOOutputs()`.
- `GPIO01AlarmMode::GPIO1_HighAlarm` is renamed `GPIO1_LowAlarm`: its code (DI06:04 = `100`) makes GPIO1 the low
  alarm output (SLAS605C Table 11), and the chip has no GPIO1 high-alarm setting. The old name stays as a
  `[[deprecated]]` alias with the same value, so existing code builds (with a warning) and behaves as before;
  `reg::GPIOProg::GPIO1_HI_ALARM` likewise becomes `GPIO1_LO_ALARM`.
- `ADS7952_CFG::MIN_VREF` / `MAX_VREF` are 2.0 V / 3.0 V and `MAX_VA` is 5.25 V, the recommended operating
  conditions of SLAS605C section 7.3 (they were 1.0 V / 2.5 V and 5.5 V). A reference between 2.5 V and 3.0 V is no
  longer converted as 2.5 V; a `vref` below 2.0 V is now clamped to 2.0 V.
- `examples/esp32/components/hf_ads7952/idf_component.yml`: `repository` and `issues` point at
  `https://github.com/hardfoc/hf-ads7952-driver` (they named a repository that does not exist).
- An installed package exports its target as `hf::ads7952`, the name the documentation and the package config
  give (it was exported as `hf::hf_ads7952`, so `find_package(hf_ads7952)` consumers failed to link the documented
  name). `hf::hf_ads7952` remains available from the package config. A host test installs the driver and builds a
  `find_package()` consumer against it, checking that every header the public headers include is installed.
- The `hf_ads7952` CMake target now puts `src/` on the include path, so code that links it finds `ads7952.ipp`;
  `cmake --install` installs the `.ipp` next to the headers.
