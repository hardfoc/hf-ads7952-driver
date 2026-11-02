# Troubleshooting

Symptoms you may see while bringing up an ADS7952, their causes and the fix for each.

Start with `RawManualFrames()`: it shows the raw replies, so it tells bus problems (no address, wrong address)
apart from analog problems (right address, wrong count). [API guide](api-guide.md#read-raw-frames) shows the call.

## Bus and protocol

| Symptom | Cause | Fix |
| --- | --- | --- |
| `ReadChannel()` returns `Error::Timeout` on every channel | The reply address never matches: wrong SPI mode, CS released between the two bytes, MISO not connected | SPI mode 0; one CS cycle per 2-byte `transfer()`; check SDO with a logic analyser |
| Every reply is `0x0000` or `0xFFFF` | No supply on +VBD, MISO floating, or CS never goes low | Measure +VBD (1.7 V to +VA); check CS on the scope |
| Addresses look right but shifted by one frame | Your port splits or merges frames, so the chip and the driver count frames differently | Exactly 16 clocks per CS low |
| `ReadAllChannels()` returns `Error::Timeout` with some channels missing | Auto-1 mask on the chip differs from the driver's (chip reset or brown-out since `EnsureInitialized()`) | `EnsureInitialized(true)` after any reset; check `valid_mask` |
| Reads return `Error::SpiError`, `bool` calls return `false` | The port's `transfer()` returned `false`: the SDK reported a bus error | Check the SPI host, pins and bus lock in the port; see [Porting](porting.md#bus-errors) |
| Every read returns `Error::NotInitialized` | `EnsureInitialized()` was never called on this object | Call it once after power-up |

## Readings

| Symptom | Cause | Fix |
| --- | --- | --- |
| Every count is 0 | MXO not connected to AINP, AINP floating, or no reference | Short MXO (pin 7) to AINP (pin 8); AINM to AGND; check REFP |
| Counts stuck at 4095 | Input above the range | Use `Range::TwoVref` (needs 2 × VREF ≤ +VA) or scale the input |
| Every channel reads about the same count | Source impedance too high: the sampling capacitor keeps the previous channel's charge | 50 Ω or less into each input, or about 150 pF at each input; or lower SCLK |
| Voltages off by a fixed factor | `vref` passed to the driver is not the voltage at REFP, or it was clamped (outside 2.0 V to 3.0 V) | Measure REFP; `SetVref()`; see [Configuration](configuration.md#reference-and-range) |
| Voltages off by exactly 2 after `SetRange()` | The chip has not latched the new range yet: `SetRange()` sends a continue frame, which the chip ignores | Call `EnterAuto1Mode()` (or another `Enter...Mode()`) after `SetRange()` |
| The first conversion after power-up is wrong | The chip's first conversion is invalid by design | `EnsureInitialized()` discards it; after a reset call `EnsureInitialized(true)` |

## Alarms and GPIOs

| Symptom | Cause | Fix |
| --- | --- | --- |
| Alarm pin never changes | GPIO0/GPIO1 not assigned an alarm function | `ProgramGPIO()` with a `GPIO01AlarmMode` other than `GPIO` |
| Alarm fires at the wrong voltage after a range change | Thresholds are stored as counts | Call `ProgramAlarmVoltage()` again after `SetRange()` or `SetVref()` |
| Deprecation warning for `GPIO1_HighAlarm` | It was misnamed: its code (DI06:04 = `100`) makes GPIO1 the **low** alarm output, and the chip has no GPIO1 high alarm | Use `GPIO1_LowAlarm` (same code); see [API guide](api-guide.md#program-alarms-and-gpios) |
| GPIO outputs do not change | GPIO not configured as output, or no mode frame since `SetGPIOOutputs()` | Set the bit in `direction_mask`; call an `Enter...Mode()` after `SetGPIOOutputs()` |
| Only GPIO0 responds | VQFN package: GPIO1 to GPIO3 do not exist | Use the TSSOP package for four GPIOs |

## Build

| Symptom | Cause | Fix |
| --- | --- | --- |
| `'ads7952.ipp' file not found` | `src/` is not on the include path | Link the `hf_ads7952` target, or add both `inc/` and `src/` |
| `hf::ads7952` not found | `add_subdirectory()` missing, or `find_package(hf_ads7952 CONFIG)` without an install | See [Installation](installation.md) |
| Errors about C++20 | Compiler older than GCC 10 / Clang 12, or the standard forced lower | The target requires `cxx_std_20`; do not override it |
