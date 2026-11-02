/**
 * @file main.cpp
 * @brief Compiles every public header from an installed copy of the driver.
 */
#include "ads7952.hpp"
#include "ads7952_version.h"

class InstalledPort : public ads7952::SpiInterface<InstalledPort> {
public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    (void)tx;
    (void)rx;
    (void)len;
    return true;
  }
};

template class ads7952::ADS7952<InstalledPort>;
