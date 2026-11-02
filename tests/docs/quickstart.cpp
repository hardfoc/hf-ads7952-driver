/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_spi.hpp"

// --8<-- [start:quickstart]
#include "ads7952.hpp"
#include <cstdio>

int read_adc(MySpi& spi) {
  ads7952::ADS7952<MySpi> adc(spi, 2.5f, 5.0f);   // REFP = 2.5 V, +VA = 5 V

  if (!adc.EnsureInitialized()) {                 // 1. discard the power-up frame, program the sequences
    return -1;
  }

  const ads7952::ReadResult ch3 = adc.ReadChannel(3);         // 2. one channel (Manual mode)
  if (!ch3.ok()) {
    return -1;                                                // Timeout: wiring, SPI mode or CS framing
  }
  std::printf("CH3 = %.3f V (%u counts)\n", ch3.voltage, static_cast<unsigned>(ch3.count));

  const ads7952::ChannelReadings all = adc.ReadAllChannels(); // 3. every channel (Auto-1 mode)
  for (uint8_t ch = 0; ch < 12; ++ch) {
    if (all.hasChannel(ch)) {
      std::printf("CH%u = %.3f V\n", static_cast<unsigned>(ch), all.voltage[ch]);
    }
  }
  return all.ok() ? 0 : -1;
}
// --8<-- [end:quickstart]
