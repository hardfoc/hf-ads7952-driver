# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses semantic versioning.

## [Unreleased]

The driver has not been released yet; its version in `cmake/hf_ads7952_build_settings.cmake` is 1.0.0.

### Added

- Host unit tests (`-DHF_ADS7952_BUILD_TESTS=ON`) against a model of the chip's SPI pipeline, and a CI workflow.

### Changed

- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, modes and the frame
  pipeline, architecture, API guide, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files are gone.

### Fixed

- `GPIO01AlarmMode::GPIO1_HighAlarm` is renamed `GPIO1_LowAlarm`: its code (DI06:04 = `100`) makes GPIO1 the low
  alarm output (SLAS605C Table 11), and the chip has no GPIO1 high-alarm setting. The old name stays as a
  `[[deprecated]]` alias with the same value, so existing code builds (with a warning) and behaves as before;
  `reg::GPIOProg::GPIO1_HI_ALARM` likewise becomes `GPIO1_LO_ALARM`.
- `ADS7952_CFG::MIN_VREF` / `MAX_VREF` are 2.0 V / 3.0 V and `MAX_VA` is 5.25 V, the recommended operating
  conditions of SLAS605C section 7.3 (they were 1.0 V / 2.5 V and 5.5 V). A reference between 2.5 V and 3.0 V is no
  longer converted as 2.5 V; a `vref` below 2.0 V is now clamped to 2.0 V.
- `examples/esp32/components/hf_ads7952/idf_component.yml`: `repository` and `issues` point at
  `https://github.com/hardfoc/hf-ads7952-driver` (they named a repository that does not exist).
- The `hf_ads7952` CMake target now puts `src/` on the include path, so code that links it finds `ads7952.ipp`;
  `cmake --install` installs the `.ipp` next to the headers.
