/**
 * @file mock_ads7952.hpp
 * @brief A model of the ADS7952's SPI behaviour for the host tests.
 *
 * The model follows SLAS605C: every frame is 16 bits; a channel selected in frame N is acquired
 * during frame N+1 and its conversion is clocked out in frame N+2, tagged with its 4-bit address in
 * DO15:12. Auto-1 walks the programmed mask, Auto-2 walks CH0..last. CONTINUE frames (DI15:12 =
 * 0000) keep the mode and ignore DI11:00 (Table 7). Range, power-down and GPIO bits are latched
 * only from mode frames with DI11 = 1 (Manual: also with DI11 = 0 for the channel address). The
 * first frame after power-up returns an invalid conversion.
 *
 * Timing of the latched settings (the reason the driver's setters send a second frame):
 * - range: the value latched by frame N applies to the input acquired in frame N+1, so the
 *   conversion clocked out in frame N+1 still used the old range (Figure 50);
 * - GPIO outputs: driven on the CS falling edge of frame N+1 from DI03:00 written in frame N,
 *   for pins whose direction bit is 1 (section 8.1, Table 11);
 * - power-down: DI05 = 1 written in frame N powers down at the 16th SCLK of frame N+1; DI05 = 0
 *   powers up on the next CS falling edge, whose conversion is invalid (8.3.2, 7.6).
 * With `analog` set, each channel converts `volts[ch]` with the range used for its acquisition.
 */
#pragma once

#include "ads7952_spi_interface.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class MockAds7952 : public ads7952::SpiInterface<MockAds7952> {
public:
  MockAds7952() noexcept {
    for (uint8_t ch = 0; ch < 12; ++ch) {
      input[ch] = static_cast<uint16_t>(100U * ch + 7U); // a distinct count on every channel
    }
  }

  // --- what the test sets --------------------------------------------------
  std::array<uint16_t, 12> input{}; ///< 12-bit count each channel converts to
  bool analog = false;              ///< convert volts[] with the acquisition's range instead
  std::array<float, 12> volts{};    ///< analog input of each channel (when analog)
  float vref = 2.5f;                ///< voltage at REFP (when analog)

  // --- what the test reads back ------------------------------------------
  std::vector<uint16_t> sent; ///< every DI15:0 word, in order
  enum class DevMode : uint8_t { Manual, Auto1, Auto2 };
  DevMode mode = DevMode::Manual;
  uint16_t auto1_mask = 0;
  uint8_t auto2_last = 0;
  bool range_2vref = false;  ///< DI06 as latched (mode-control register)
  bool power_down = false;   ///< DI05 as latched (mode-control register)
  uint8_t gpio_out_reg = 0;  ///< DI03:00 as latched (mode-control register)
  uint8_t gpio_pins = 0;     ///< levels driven on the GPIO output pins right now
  bool powered_down = false; ///< the device is in power-down right now
  uint16_t gpio_prog = 0; ///< last GPIO program register word (DI09:00)
  std::array<uint16_t, 12> alarm_high{};

  // GPIO0/GPIO1 functions decoded from DI06:04 of the GPIO program register (Table 11).
  uint16_t alarmCode() const {
    return static_cast<uint16_t>((gpio_prog >> 4) & 0x7);
  }
  bool gpio0HighOrLowAlarm() const {
    return (alarmCode() & 0x1) != 0;
  } // xx1
  bool gpio0HighAlarm() const {
    return alarmCode() == 0x2 || alarmCode() == 0x6;
  } // 010, 110
  bool gpio1LowAlarm() const {
    return alarmCode() == 0x4 || alarmCode() == 0x6;
  } // 100, 110
  std::array<uint16_t, 12> alarm_low{};

  // --- bus fault injection ------------------------------------------------
  /// Number of transfers that still succeed; once it reaches 0 every transfer fails (the frame is
  /// not clocked: the device does not see it). Negative: never fail.
  int transfers_before_failure = -1;
  std::size_t failed_transfers = 0; ///< transfers that reported a bus error

  // CRTP contract: one full-duplex transfer, MSB first; false on a bus error.
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    if (transfers_before_failure == 0) {
      ++failed_transfers;
      return false;
    }
    if (transfers_before_failure > 0) {
      --transfers_before_failure;
    }
    if (len != 2 || tx == nullptr || rx == nullptr) {
      return false;
    }
    const uint16_t di = static_cast<uint16_t>((tx[0] << 8) | tx[1]);
    const uint16_t dout = frame(di);
    rx[0] = static_cast<uint8_t>(dout >> 8);
    rx[1] = static_cast<uint8_t>(dout & 0xFF);
    return true;
  }

private:
  enum class Prog : uint8_t { None, Auto1Mask, Alarm };

  uint16_t frame(uint16_t di) {
    sent.push_back(di);

    // CS falling edge: GPO pins take DI03:00 of the previous frame (outputs only); DI05 = 0 wakes
    // the device, and that frame's conversion is invalid.
    gpio_pins = static_cast<uint8_t>(gpio_out_reg & gpio_prog & 0x0F);
    bool invalid = first_frame_;
    if (powered_down && !power_down) {
      powered_down = false;
      invalid = true;
    }
    const bool power_down_at_end = power_down;

    // Output: the channel that was acquired during the previous frame, sampled at this CS falling
    // edge, converted with the range that applied to that acquisition.
    uint16_t dout = invalid
                        ? 0x0000
                        : static_cast<uint16_t>((acquiring_ << 12) |
                                                (convert(acquiring_, acquiring_2vref_) & 0x0FFF));
    first_frame_ = false;
    acquiring_ = pending_;
    acquiring_2vref_ = range_2vref; // this frame's acquisition uses the range latched so far
    dout = process(di, dout);
    if (power_down_at_end) {
      powered_down = true; // 16th SCLK falling edge of the frame after DI05 = 1
    }
    return dout;
  }

  uint16_t convert(uint8_t ch, bool two_vref) const {
    if (!analog) {
      return input[ch];
    }
    const float fs = vref * (two_vref ? 2.0f : 1.0f);
    const float c = volts[ch] / fs * 4095.0f + 0.5f;
    return static_cast<uint16_t>(c < 0.0f ? 0.0f : (c > 4095.0f ? 4095.0f : c));
  }

  uint16_t process(uint16_t di, uint16_t dout) {
    // Programming sequences in progress take the frame; the device keeps converting in its mode.
    if (prog_ == Prog::Auto1Mask) {
      auto1_mask = di & 0x0FFF;
      prog_ = Prog::None;
      advance();
      return dout;
    }
    if (prog_ == Prog::Alarm) {
      const uint8_t ch = static_cast<uint8_t>(alarm_group_ * 4 + ((di >> 14) & 0x3));
      if (ch < 12) {
        ((di & 0x2000) ? alarm_high : alarm_low)[ch] = di & 0x03FF;
      }
      if (di & 0x1000) {
        prog_ = Prog::None;
      }
      advance();
      return dout;
    }

    const uint8_t code = static_cast<uint8_t>(di >> 12);
    const bool program = (di & 0x0800) != 0;
    switch (code) {
    case 0x0: // CONTINUE: DI11:00 ignored
      advance();
      break;
    case 0x1: // Manual: channel address always applies
      mode = DevMode::Manual;
      pending_ = static_cast<uint8_t>((di >> 7) & 0x0F) % 12;
      if (program)
        latch(di);
      break;
    case 0x2: // Auto-1
      mode = DevMode::Auto1;
      if (program) {
        latch(di);
        if (di & 0x0400)
          pending_ = nextInMask(0, true);
        else
          advance();
      } else {
        advance();
      }
      break;
    case 0x3: // Auto-2
      mode = DevMode::Auto2;
      if (program) {
        latch(di);
        if (di & 0x0400)
          pending_ = 0;
        else
          advance();
      } else {
        advance();
      }
      break;
    case 0x4: // GPIO program register
      gpio_prog = di & 0x03FF;
      advance();
      break;
    case 0x8: // Auto-1 program: mask in the next frame
      prog_ = Prog::Auto1Mask;
      advance();
      break;
    case 0x9: // Auto-2 program: last channel in DI09:06
      auto2_last = static_cast<uint8_t>((di >> 6) & 0x0F);
      advance();
      break;
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
      prog_ = Prog::Alarm;
      alarm_group_ = static_cast<uint8_t>(code & 0x3);
      advance();
      break;
    default:
      advance();
      break;
    }
    return dout;
  }

  void latch(uint16_t di) {
    range_2vref = (di & 0x0040) != 0;
    power_down = (di & 0x0020) != 0;
    gpio_out_reg = static_cast<uint8_t>(di & 0x000F);
  }

  void advance() {
    switch (mode) {
    case DevMode::Manual:
      break; // keeps the last selected channel
    case DevMode::Auto1:
      pending_ = nextInMask(static_cast<uint8_t>(pending_ + 1), false);
      break;
    case DevMode::Auto2:
      pending_ = (pending_ >= auto2_last) ? 0 : static_cast<uint8_t>(pending_ + 1);
      break;
    }
  }

  uint8_t nextInMask(uint8_t from, bool reset) const {
    const uint16_t mask = auto1_mask & 0x0FFF;
    if (mask == 0)
      return 0;
    for (uint8_t i = 0; i < 12; ++i) {
      const uint8_t ch = static_cast<uint8_t>((reset ? i : from + i) % 12);
      if (mask & (1U << ch))
        return ch;
    }
    return 0;
  }

  bool first_frame_ = true;
  uint8_t acquiring_ = 0;
  bool acquiring_2vref_ = false;
  uint8_t pending_ = 0;
  Prog prog_ = Prog::None;
  uint8_t alarm_group_ = 0;
};
