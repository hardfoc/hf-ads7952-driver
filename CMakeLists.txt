#===============================================================================
# ADS7952 Driver - Root CMake
# Defines the main build target for the driver. Minimal, platform-agnostic.
#===============================================================================

cmake_minimum_required(VERSION 3.16)

project(hf_ads7952_driver LANGUAGES CXX)

#===============================================================================
# Load shared build settings (target name, includes, sources, dependencies)
#===============================================================================
include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_ads7952_build_settings.cmake)

#===============================================================================
# Target type selection
# INTERFACE for header-only, STATIC if sources present
#===============================================================================
if(HF_ADS7952_SOURCE_FILES)
    add_library(${HF_ADS7952_TARGET_NAME} STATIC ${HF_ADS7952_SOURCE_FILES})
    target_include_directories(${HF_ADS7952_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
            $<BUILD_INTERFACE:${HF_ADS7952_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_ADS7952_TARGET_NAME} PUBLIC cxx_std_20)
else()
    add_library(${HF_ADS7952_TARGET_NAME} INTERFACE)
    target_include_directories(${HF_ADS7952_TARGET_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
            $<BUILD_INTERFACE:${HF_ADS7952_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_ADS7952_TARGET_NAME} INTERFACE cxx_std_20)
endif()

# =============================================================================
# Namespaced Consumer Alias
# =============================================================================
add_library(hf::ads7952 ALIAS ${HF_ADS7952_TARGET_NAME})
# Installed copies export the same name: hf:: + EXPORT_NAME = hf::ads7952 (was hf::hf_ads7952).
set_target_properties(${HF_ADS7952_TARGET_NAME} PROPERTIES EXPORT_NAME ads7952)

#===============================================================================
# Versioning
# Version variables and header generation are defined in:
#   cmake/hf_ads7952_build_settings.cmake (single source of truth)
#===============================================================================
set_target_properties(${HF_ADS7952_TARGET_NAME} PROPERTIES VERSION ${HF_ADS7952_VERSION} SOVERSION ${HF_ADS7952_VERSION_MAJOR})

#===============================================================================
# Optional: Enable extra warnings (can be toggled by user)
#===============================================================================
option(HF_ADS7952_ENABLE_WARNINGS "Enable extra warnings for ADS7952 driver" OFF)
if(HF_ADS7952_ENABLE_WARNINGS)
    if(TARGET ${HF_ADS7952_TARGET_NAME})
        target_compile_options(${HF_ADS7952_TARGET_NAME} INTERFACE -Wall -Wextra -Wpedantic)
    endif()
endif()

# ── Host unit tests (mock SPI port; no hardware) ─────────────────────────────
option(HF_ADS7952_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_ADS7952_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

#===============================================================================
# Install and export support (for find_package usage)
#===============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${HF_ADS7952_TARGET_NAME} EXPORT hf_ads7952Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.h.in" EXCLUDE)
install(FILES "${HF_ADS7952_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES src/ads7952.ipp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT hf_ads7952Targets
    FILE hf_ads7952Targets.cmake
    NAMESPACE hf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_ads7952
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hf_ads7952Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_ads7952Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_ads7952
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hf_ads7952ConfigVersion.cmake"
    VERSION ${HF_ADS7952_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hf_ads7952Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_ads7952ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_ads7952
)
