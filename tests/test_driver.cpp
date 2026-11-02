/**
 * @file test_driver.cpp
 * @brief Host tests: the driver against the ADS7952 model in mock_ads7952.hpp.
 */
#include "ads7952.hpp"
#include "mock_ads7952.hpp"

#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
  ++g_checks;
  if (!ok) {
    ++g_failures;
    std::printf("FAIL line %d: %s\n", line, what);
  }
}

#define CHECK(x) check((x), #x, __LINE__)

using Adc = ads7952::ADS7952<MockAds7952>;

void test_init_programs_defaults() {
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(!adc.IsInitialized());
  CHECK(adc.EnsureInitialized());
  CHECK(adc.IsInitialized());
  CHECK(chip.auto1_mask == 0x0FFF);
  CHECK(chip.auto2_last == 11);
  CHECK(chip.mode == MockAds7952::DevMode::Auto1);
  CHECK(adc.GetMode() == ads7952::Mode::Auto1);
  const auto frames = chip.sent.size();
  CHECK(adc.EnsureInitialized());          // idempotent: no SPI traffic
  CHECK(chip.sent.size() == frames);
  CHECK(adc.EnsureInitialized(true));      // forced: runs again
  CHECK(chip.sent.size() > frames);
}

void test_read_channel_keys_on_address() {
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(adc.EnsureInitialized());
  for (uint8_t ch = 0; ch < 12; ++ch) {
    const auto r = adc.ReadChannel(ch);
    CHECK(r.ok());
    CHECK(r.channel == ch);
    CHECK(r.count == chip.input[ch]);
    CHECK(std::fabs(r.voltage - chip.input[ch] * 2.5f / 4095.0f) < 1e-4f);
  }
  CHECK(adc.GetMode() == ads7952::Mode::Manual);
}

void test_read_channel_errors() {
  MockAds7952 chip;
  Adc adc(chip);
  CHECK(adc.ReadChannel(0).error == ads7952::Error::NotInitialized);
  CHECK(adc.ReadAllChannels().error == ads7952::Error::NotInitialized);
  CHECK(adc.EnsureInitialized());
  CHECK(adc.ReadChannel(12).error == ads7952::Error::InvalidChannel);
}

void test_read_all_channels() {
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(adc.EnsureInitialized());
  const auto all = adc.ReadAllChannels();
  CHECK(all.ok());
  CHECK(all.valid_mask == 0x0FFF);
  CHECK(all.validChannelCount() == 12);
  for (uint8_t ch = 0; ch < 12; ++ch) {
    CHECK(all.hasChannel(ch));
    CHECK(all.count[ch] == chip.input[ch]);
  }
}

void test_read_subset() {
  MockAds7952 chip;
  Adc adc(chip);
  CHECK(adc.EnsureInitialized());
  CHECK(adc.ProgramAuto1Channels(ads7952::ChannelMask(1, 5, 9)));
  CHECK(chip.auto1_mask == 0x0222);
  const auto r = adc.ReadAllChannels();
  CHECK(r.ok());
  CHECK(r.valid_mask == 0x0222);
  CHECK(r.count[5] == chip.input[5]);
  CHECK(!r.hasChannel(0));
}

void test_programming_frames() {
  MockAds7952 chip;
  Adc adc(chip);
  CHECK(adc.EnsureInitialized());
  CHECK(adc.ProgramAuto2LastChannel(5));
  CHECK(chip.auto2_last == 5);
  CHECK(!adc.ProgramAuto2LastChannel(12));

  CHECK(adc.ProgramAlarm(6, ads7952::AlarmBound::High, 3000));
  CHECK(chip.alarm_high[6] == (3000 >> 2));
  CHECK(adc.ProgramAlarm(9, ads7952::AlarmBound::Low, 400));
  CHECK(chip.alarm_low[9] == (400 >> 2));
  CHECK(!adc.ProgramAlarm(12, ads7952::AlarmBound::Low, 0));

  ads7952::GPIOConfig cfg;
  cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO0_HighAndLowAlarm;
  cfg.direction_mask = ads7952::gpio::kGPIO2 | ads7952::gpio::kGPIO3;
  CHECK(adc.ProgramGPIO(cfg));
  CHECK(chip.gpio_prog == (0x0010 | 0x000C));
}

void test_range_latches_on_mode_frame() {
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(adc.EnsureInitialized());
  CHECK(adc.SetRange(ads7952::Range::TwoVref));
  CHECK(std::fabs(adc.GetActiveVref() - 5.0f) < 1e-6f);
  CHECK(adc.EnterAuto1Mode());             // the documented way to latch the range bit
  CHECK(chip.range_2vref);
}

void test_conversion_helpers() {
  static_assert(ads7952::ChannelMask(0, 2, 4) == 0x0015);
  static_assert(ads7952::ChannelRangeMask(0, 5) == 0x003F);
  static_assert(ads7952::ChannelRangeMask(5, 4) == 0);
  static_assert(ads7952::VoltageToCount(0.5f, 2.5f) == 819);
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(adc.VoltageToCount(2.5f) == 4095);
  CHECK(adc.VoltageToCount(-1.0f) == 0);
  CHECK(std::fabs(adc.CountToVoltage(4095) - 2.5f) < 1e-6f);
}

void test_raw_frames() {
  MockAds7952 chip;
  Adc adc(chip);
  uint16_t words[4] = {};
  CHECK(adc.RawManualFrames(3, words, 4) == 0);  // not initialised
  CHECK(adc.EnsureInitialized());
  CHECK(adc.RawManualFrames(3, words, 4) == 4);
  CHECK((words[2] >> 12) == 3);                 // N + 2
  CHECK((words[2] & 0x0FFF) == chip.input[3]);
}

}  // namespace

int main() {
  test_init_programs_defaults();
  test_read_channel_keys_on_address();
  test_read_channel_errors();
  test_read_all_channels();
  test_read_subset();
  test_programming_frames();
  test_range_latches_on_mode_frame();
  test_conversion_helpers();
  test_raw_frames();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
