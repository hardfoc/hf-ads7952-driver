/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the documentation's Quickstart and Porting pages, compiled by the host build.
 *
 * The one platform call goes through `platform_spi_transfer()`, declared below; a real port replaces it with its
 * SDK (ESP-IDF, STM32 HAL, Zephyr, Linux spidev). The class is what the driver needs, nothing more.
 */
#pragma once

#include "ads7952_spi_interface.hpp"

#include <cstddef>
#include <cstdint>

// Platform hook: your SDK's call. Declared only, so the docs build checks the port compiles.
bool platform_spi_transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) noexcept;  // CS low .. CS high

// --8<-- [start:port]
class MySpi : public ads7952::SpiInterface<MySpi> {
public:
  // One 16-bit frame: SPI mode 0, MSB first, CS low for both bytes and high again afterwards.
  // Return false on a bus error: the driver call that sent the frame then fails.
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    return platform_spi_transfer(tx, rx, len);
  }
};
// --8<-- [end:port]
