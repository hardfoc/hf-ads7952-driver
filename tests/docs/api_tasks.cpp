/**
 * @file api_tasks.cpp
 * @brief The API guide's and Configuration page's snippets, compiled by the host build (never run).
 */
#include "ads7952.hpp"
#include "my_spi.hpp"

#include <cstdio>

using Adc = ads7952::ADS7952<MySpi>;

bool configure(Adc& adc) {
  // --8<-- [start:configure]
  // Auto-1 scans any subset; Auto-2 scans CH0..last. Program them once after EnsureInitialized().
  if (!adc.ProgramAuto1Channels(ads7952::ChannelMask(0, 1, 4, 5))) {
    return false;
  }
  if (!adc.ProgramAuto2LastChannel(7)) {               // CH0..CH7
    return false;
  }

  // 0..2 x VREF: only valid while 2 x VREF <= +VA (2.5 V reference, 5 V supply).
  if (!adc.SetRange(ads7952::Range::TwoVref)) {        // in effect on the chip when it returns
    return false;
  }
  // --8<-- [end:configure]
  return true;
}

void read(Adc& adc) {
  // --8<-- [start:read]
  // One channel: Manual mode, three frames, keyed on the channel address in the reply.
  const ads7952::ReadResult r = adc.ReadChannel(4);
  if (r.ok()) {
    std::printf("CH%u: %u counts, %.3f V\n", static_cast<unsigned>(r.channel),
                static_cast<unsigned>(r.count), r.voltage);
  }

  // Every channel of the Auto-1 mask: one frame per channel plus two.
  const ads7952::ChannelReadings all = adc.ReadAllChannels();
  for (uint8_t ch = 0; ch < ads7952::ChannelReadings::MAX_CHANNELS; ++ch) {
    if (all.hasChannel(ch)) {
      std::printf("CH%u: %.3f V\n", static_cast<unsigned>(ch), all.voltage[ch]);
    }
  }
  if (!all.ok()) {
    std::printf("%u of the programmed channels arrived\n", static_cast<unsigned>(all.validChannelCount()));
  }
  // --8<-- [end:read]
}

bool alarms(Adc& adc) {
  // --8<-- [start:alarms]
  // GPIO0 goes high while any channel is outside its window (active-high output).
  ads7952::GPIOConfig gpio;
  gpio.alarm_mode = ads7952::GPIO01AlarmMode::GPIO0_HighAndLowAlarm;
  gpio.direction_mask = ads7952::gpio::kGPIO2 | ads7952::gpio::kGPIO3;  // GPIO2, GPIO3: outputs
  if (!adc.ProgramGPIO(gpio)) {
    return false;
  }

  // A window of 0.5 V .. 2.0 V on CH0, from volts with the active reference ...
  if (!adc.ProgramAlarmVoltage(0, ads7952::AlarmBound::Low, 0.5f) ||
      !adc.ProgramAlarmVoltage(0, ads7952::AlarmBound::High, 2.0f)) {
    return false;
  }
  // ... or from counts. The chip keeps the top 10 bits (4-count steps).
  if (!adc.ProgramAlarm(1, ads7952::AlarmBound::High, 3000)) {
    return false;
  }
  // --8<-- [end:alarms]
  return true;
}

bool outputs_and_power(Adc& adc) {
  // --8<-- [start:write]
  // Each call is in effect on the chip when it returns.
  if (!adc.SetGPIOOutputs(ads7952::gpio::kGPIO2)) {    // GPIO2 high, GPIO3 low
    return false;
  }
  if (!adc.SetPowerDown(ads7952::PowerDown::PowerDown)) {  // the chip is powered down
    return false;
  }
  if (!adc.SetPowerDown(ads7952::PowerDown::Normal)) {     // awake again
    return false;
  }
  // --8<-- [end:write]
  return true;
}

void errors(Adc& adc) {
  // --8<-- [start:errors]
  const ads7952::ReadResult r = adc.ReadChannel(2);
  switch (r.error) {
  case ads7952::Error::Ok:
    break;
  case ads7952::Error::NotInitialized:                 // call EnsureInitialized() first
    adc.EnsureInitialized();
    break;
  case ads7952::Error::InvalidChannel:                 // channel > 11
    break;
  case ads7952::Error::Timeout:                        // the address never came back: check the bus
    adc.EnsureInitialized(true);                       // after a reset or brown-out, program again
    break;
  case ads7952::Error::SpiError:                       // the port's transfer() returned false
    break;
  default:
    break;
  }
  // --8<-- [end:errors]
}

void raw(Adc& adc) {
  // --8<-- [start:raw]
  // Bring-up: clock frames by hand and see the address pipeline (CH in DO15:12, count in DO11:0).
  uint16_t words[4] = {};
  const uint8_t n = adc.RawManualFrames(6, words, 4);
  for (uint8_t i = 0; i < n; ++i) {
    std::printf("frame %u: CH%u = %u\n", static_cast<unsigned>(i),
                static_cast<unsigned>(words[i] >> 12), static_cast<unsigned>(words[i] & 0x0FFF));
  }
  // --8<-- [end:raw]
}

void calibrate(Adc& adc, float measured_vref) {
  // --8<-- [start:calibrate]
  // Software only: the scale used by CountToVoltage(), VoltageToCount() and the results.
  adc.SetVref(measured_vref);                          // e.g. 2.497 V measured at REFP
  const float v = adc.CountToVoltage(2048);            // with the active reference (range applied)
  const uint16_t c = ads7952::VoltageToCount(1.25f, adc.GetActiveVref());
  std::printf("2048 counts = %.4f V, 1.25 V = %u counts\n", v, static_cast<unsigned>(c));
  // --8<-- [end:calibrate]
}
