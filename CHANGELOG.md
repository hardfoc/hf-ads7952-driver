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

- `examples/esp32/components/hf_ads7952/idf_component.yml`: `repository` and `issues` point at
  `https://github.com/hardfoc/hf-ads7952-driver` (they named a repository that does not exist).
- The `hf_ads7952` CMake target now puts `src/` on the include path, so code that links it finds `ads7952.ipp`;
  `cmake --install` installs the `.ipp` next to the headers.
