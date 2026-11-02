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

// A port written for the earlier contract: transfer() returns void. Still accepted, never fails.
class LegacyVoidPort : public ads7952::SpiInterface<LegacyVoidPort> {
public:
  MockAds7952 chip;
  void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    (void)chip.transfer(tx, rx, len);
  }
};

static_assert(ads7952::detail::kTransferReportsStatus<MockAds7952>);
static_assert(!ads7952::detail::kTransferReportsStatus<LegacyVoidPort>);

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
  CHECK(adc.EnsureInitialized()); // idempotent: no SPI traffic
  CHECK(chip.sent.size() == frames);
  CHECK(adc.EnsureInitialized(true)); // forced: runs again
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

// SLAS605C Table 11, DI06:04: 100 = GPIO1 low alarm, 110 = GPIO1 low + GPIO0 high alarm.
void test_gpio_alarm_routing() {
  MockAds7952 chip;
  Adc adc(chip);
  CHECK(adc.EnsureInitialized());
  ads7952::GPIOConfig cfg;
  cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO1_LowAlarm;
  CHECK(adc.ProgramGPIO(cfg));
  CHECK(chip.alarmCode() == 0x4);
  CHECK(chip.gpio1LowAlarm());
  CHECK(!chip.gpio0HighAlarm() && !chip.gpio0HighOrLowAlarm());

  cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO0_HighAlarm;
  CHECK(adc.ProgramGPIO(cfg));
  CHECK(chip.gpio0HighAlarm() && !chip.gpio1LowAlarm());

  cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO1_LowAlarm_GPIO0_HighAlarm;
  CHECK(adc.ProgramGPIO(cfg));
  CHECK(chip.gpio0HighAlarm() && chip.gpio1LowAlarm());

  cfg.alarm_mode = ads7952::GPIO01AlarmMode::GPIO0_HighAndLowAlarm;
  CHECK(adc.ProgramGPIO(cfg));
  CHECK(chip.gpio0HighOrLowAlarm() && !chip.gpio1LowAlarm());
}

void test_range_latches_on_mode_frame() {
  MockAds7952 chip;
  Adc adc(chip, 2.5f, 5.0f);
  CHECK(adc.EnsureInitialized());
  CHECK(adc.SetRange(ads7952::Range::TwoVref));
  CHECK(std::fabs(adc.GetActiveVref() - 5.0f) < 1e-6f);
  CHECK(adc.EnterAuto1Mode()); // the documented way to latch the range bit
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

// SLAS605C 7.3 Recommended Operating Conditions: V(REF) 2.0 V .. 3.0 V, +VA 2.7 V .. 5.25 V.
void test_reference_and_supply_limits() {
  MockAds7952 chip;
  Adc adc(chip, 3.0f, 5.25f);
  CHECK(std::fabs(adc.GetVref() - 3.0f) < 1e-6f); // 3.0 V is in range, not clamped to 2.5 V
  CHECK(std::fabs(adc.GetVA() - 5.25f) < 1e-6f);
  CHECK(std::fabs(adc.CountToVoltage(4095) - 3.0f) < 1e-6f);
  adc.SetVref(1.5f); // below the 2.0 V minimum
  CHECK(std::fabs(adc.GetVref() - 2.0f) < 1e-6f);
  adc.SetVref(3.3f); // above the 3.0 V maximum
  CHECK(std::fabs(adc.GetVref() - 3.0f) < 1e-6f);
  adc.SetVA(5.5f); // above the 5.25 V maximum
  CHECK(std::fabs(adc.GetVA() - 5.25f) < 1e-6f);
  adc.SetVA(2.0f); // below the 2.7 V minimum
  CHECK(std::fabs(adc.GetVA() - 2.7f) < 1e-6f);
  static_assert(ADS7952_CFG::MIN_VREF == 2.0f && ADS7952_CFG::MAX_VREF == 3.0f);
  static_assert(ADS7952_CFG::MIN_VA == 2.7f && ADS7952_CFG::MAX_VA == 5.25f);
}

// A bus error from the port's transfer() reaches the caller on every path.
void test_bus_errors_propagate() {
  using ads7952::Error;
  {
    MockAds7952 chip;
    Adc adc(chip);
    chip.transfers_before_failure = 0;
    CHECK(!adc.EnsureInitialized()); // the power-up discard frame already fails
    CHECK(!adc.IsInitialized());
  }
  for (int good = 1; good < 5; ++good) { // fail each of the 5 init frames in turn
    MockAds7952 chip;
    Adc adc(chip);
    chip.transfers_before_failure = good;
    CHECK(!adc.EnsureInitialized());
    CHECK(!adc.IsInitialized());
  }

  MockAds7952 chip;
  Adc adc(chip);
  CHECK(adc.EnsureInitialized());
  auto fail_after = [&](int n) { chip.transfers_before_failure = n; };

  fail_after(0);
  CHECK(adc.ReadChannel(3).error == Error::SpiError);
  fail_after(1); // Manual frame passes, first CONTINUE fails
  CHECK(adc.ReadChannel(3).error == Error::SpiError);
  fail_after(0);
  CHECK(adc.ReadAllChannels().error == Error::SpiError);
  fail_after(3); // Auto-1 frame and two CONTINUEs pass
  CHECK(adc.ReadAllChannels().error == Error::SpiError);

  fail_after(0);
  CHECK(!adc.EnterManualMode(2));
  CHECK(!adc.EnterAuto1Mode());
  CHECK(!adc.EnterAuto2Mode());
  CHECK(!adc.ProgramAuto2LastChannel(4));
  CHECK(adc.GetAuto2LastChannel() == 11); // unchanged
  CHECK(!adc.ProgramAuto1Channels(0x0003));
  fail_after(1); // the mask frame fails
  CHECK(!adc.ProgramAuto1Channels(0x0003));
  CHECK(adc.GetAuto1ChannelMask() == 0x0FFF); // unchanged
  fail_after(0);
  CHECK(!adc.ProgramGPIO(ads7952::GPIOConfig{}));
  for (int good = 0; good < 3; ++good) { // each of the three alarm frames
    fail_after(good);
    CHECK(!adc.ProgramAlarm(5, ads7952::AlarmBound::High, 2000));
  }
  fail_after(0);
  CHECK(!adc.SetGPIOOutputs(ads7952::gpio::kGPIO2));
  CHECK(!adc.SetPowerDown(ads7952::PowerDown::PowerDown));
  CHECK(!adc.SetRange(ads7952::Range::TwoVref));

  uint16_t words[4] = {};
  fail_after(0);
  CHECK(adc.RawManualFrames(3, words, 4) == 0);
  fail_after(2);
  CHECK(adc.RawManualFrames(3, words, 4) == 2); // frames 0 and 1 arrived

  fail_after(-1); // the bus recovers: everything works again
  CHECK(adc.ReadChannel(3).ok());
  CHECK(chip.failed_transfers > 0);
}

// A void-returning port still builds and works; it can never report an error.
void test_legacy_void_port() {
  LegacyVoidPort port;
  ads7952::ADS7952<LegacyVoidPort> adc(port, 2.5f, 5.0f);
  CHECK(adc.EnsureInitialized());
  const auto r = adc.ReadChannel(7);
  CHECK(r.ok());
  CHECK(r.count == port.chip.input[7]);
  CHECK(adc.ReadAllChannels().ok());
}

void test_raw_frames() {
  MockAds7952 chip;
  Adc adc(chip);
  uint16_t words[4] = {};
  CHECK(adc.RawManualFrames(3, words, 4) == 0); // not initialised
  CHECK(adc.EnsureInitialized());
  CHECK(adc.RawManualFrames(3, words, 4) == 4);
  CHECK((words[2] >> 12) == 3); // N + 2
  CHECK((words[2] & 0x0FFF) == chip.input[3]);
}

} // namespace

int main() {
  test_init_programs_defaults();
  test_read_channel_keys_on_address();
  test_read_channel_errors();
  test_read_all_channels();
  test_read_subset();
  test_programming_frames();
  test_gpio_alarm_routing();
  test_range_latches_on_mode_frame();
  test_conversion_helpers();
  test_reference_and_supply_limits();
  test_bus_errors_propagate();
  test_legacy_void_port();
  test_raw_frames();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
