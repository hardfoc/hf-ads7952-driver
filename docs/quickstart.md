# Quickstart

Read the ADS7952 from your MCU: wire the SPI bus and the analog front end, write a one-method port class, call
three driver functions.

## 1. Wire it

| MCU | ADS7952 (TSSOP-38 pin) | Notes |
| --- | --- | --- |
| SPI SCLK | SCLK (32) | SPI mode 0, up to 20 MHz; start at 1 MHz to 4 MHz |
| SPI MOSI | SDI (33) | 16-bit command word, MSB first |
| SPI MISO | SDO (34) | 16-bit reply: channel address in DO15:12, count in DO11:0 |
| SPI CS | CS (31) | Active low; one CS frame per 16-bit word |
| I/O supply | +VBD (36), BDGND (35) | 1.7 V to +VA; sets the logic levels, so match your MCU |
| GND | AGND, BDGND | Common ground |

On the analog side, connect **MXO (7) to AINP (8)** and AINM (9) to AGND: the multiplexer output is not routed to
the converter inside the chip. Put a 2.0 V to 3.0 V reference on REFP (4) with REFM (3) to AGND, and +VA
(5, 29) at 2.7 V to 5.25 V. The [Hardware](hardware.md) page has every pin, the bus timing and the power ranges.

## 2. Add the driver

Add the repository as a submodule and link the `hf_ads7952` CMake target (or add `inc/` and `src/` to your include
path and build with C++20). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-ads7952-driver.git external/hf-ads7952-driver
```

## 3. Write the port

The port is the only platform code: one full-duplex transfer with chip select held low for the whole word. Replace
`platform_spi_transfer()` with your SDK's call.

```cpp title="my_spi.hpp"
--8<-- "tests/docs/my_spi.hpp:port"
```

The driver always calls `transfer()` with two bytes. [Porting](porting.md) explains the contract and shows how to
check a port.

## 4. Read the channels

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. The constructor stores the reference (REFP) and supply (+VA) voltages used to convert counts to volts. It sends
   nothing.
2. `EnsureInitialized()` discards the invalid first conversion after power-up, programs the Auto-1 sequence (all 12
   channels) and the Auto-2 last channel (CH11), and enters Auto-1 mode. Calling it again does nothing.
3. `ReadChannel()` selects one channel in Manual mode and clocks frames until the reply carries that channel's
   address: three frames when the bus is healthy.
4. `ReadAllChannels()` restarts the Auto-1 sequence and collects one result per programmed channel.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `ReadChannel()` returns `Error::Timeout` | SPI mode 0, CS low for all 16 clocks and high between words, MISO wiring |
| Every count is 0 | +VA and the reference are present; MXO is connected to AINP |
| Every count is 4095 | The input is above the range: VREF, or 2 × VREF in `Range::TwoVref` |
| Every channel reads the same count | MXO to AINP wiring, source impedance; see [Troubleshooting](troubleshooting.md) |

## Next

- [API guide](api-guide.md): sequences, ranges, alarms, GPIOs and errors.
- [Modes and the frame pipeline](modes-and-pipeline.md): why a result arrives two frames after its command.
- [Examples](examples.md): ESP32-S3 applications for every feature.
