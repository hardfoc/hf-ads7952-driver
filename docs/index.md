# HF-ADS7952 Documentation

Welcome! This documentation covers everything you need to install, configure, and use the **HF-ADS7952** driver — a portable C++20 driver for the Texas Instruments ADS7952 12-channel, 12-bit SAR ADC.

## Documentation Structure

### **Getting Started**

1. **[🛠️ Installation](installation.md)** — Prerequisites, obtaining the source, CMake setup
2. **[⚡ Quick Start](quickstart.md)** — Minimal working example in under 20 lines
3. **[🔌 Hardware Setup](hardware_setup.md)** — Wiring, SPI configuration, power supply, decoupling

### **Integration**

4. **[🔧 Platform Integration](platform_integration.md)** — Implement the CRTP SPI interface for ESP32, STM32, Linux
5. **[⚙️ Configuration](configuration.md)** — Kconfig, CMake defines, voltage references, channel masks, power modes
6. **[🔩 CMake Integration](cmake_integration.md)** — Build system details, ESP-IDF component, FetchContent

### **Reference**

7. **[📖 API Reference](api_reference.md)** — Complete method signatures, enums, structs, error codes
8. **[💡 Examples](examples.md)** — Walkthrough of all 4 example applications

### **Troubleshooting**

9. **[🐛 Troubleshooting](troubleshooting.md)** — Error codes, SPI debugging, common hardware issues

---

## Quick Start Path

**New to ADS7952?** Follow this recommended learning path:

1. **[Installation](installation.md)** — Get the library into your project
2. **[Hardware Setup](hardware_setup.md)** — Wire the ADS7952 to your MCU
3. **[Quick Start](quickstart.md)** — Read your first ADC channel
4. **[Platform Integration](platform_integration.md)** — Implement the SPI interface for your platform
5. **[Configuration](configuration.md)** — Customize voltage refs, channel masks, and modes
6. **[Examples](examples.md)** — Explore multi-mode, alarm, and GPIO examples

---

## Architecture Overview

```
┌──────────────────────────────────────────────┐
│        Your Application Code                 │
│  adc.ReadChannel(3)  /  adc.ReadAllChannels()│
└──────────────┬───────────────────────────────┘
               │
┌──────────────▼───────────────────────────────┐
│  ADS7952<SpiType>                            │  ← Driver (platform-agnostic)
│  - 16-bit SPI frame construction             │
│  - SPI pipeline (2-frame read sequence)      │
│  - Auto-1/Auto-2/Manual mode management      │
│  - Alarm + GPIO programming                  │
│  - Voltage ref tracking & conversion         │
└──────────────┬───────────────────────────────┘
               │ spiTransfer16() → transfer()
┌──────────────▼───────────────────────────────┐
│  SpiInterface<Derived>  (CRTP)               │  ← Zero-overhead abstraction
└──────────────┬───────────────────────────────┘
               │ compile-time dispatch
┌──────────────▼───────────────────────────────┐
│  Your Platform SPI  (ESP32 / STM32 / Linux)  │  ← Your implementation
│  - transfer(tx, rx, len)                     │
└──────────────────────────────────────────────┘
```

The driver handles the ADS7952's **SPI pipeline** internally — the response to SPI frame N contains the conversion result triggered by frame N−1. All read methods issue the correct number of frames and discard stale data automatically.

---

## Device Quick Reference

| Parameter | Value |
|-----------|-------|
| Resolution | 12-bit (0–4095) |
| Channels | 12 analog inputs |
| Throughput | Up to 1 MSPS |
| SPI Interface | 16-bit, full-duplex, Mode 0, MSB first |
| SPI Clock | Up to 20 MHz |
| Reference | External (typically 2.5 V) |
| Supply (VA) | 2.7–5.25 V |
| Input Range | 0–Vref or 0–2×Vref |
| GPIO | 4 pins (configurable: I/O, alarm, range, power-down) |
| Alarms | Per-channel high/low thresholds (10-bit resolution) |
| Datasheet | TI SLAS605C Rev C |

---

## Need Help?

- **🐛 Something not working?** Check the [Troubleshooting](troubleshooting.md) guide
- **❓ API questions?** Review the [API Reference](api_reference.md)
- **💬 Want to contribute?** See the contributing guidelines in the main [README](../README.md)

---

**Navigation**
➡️ [Installation](installation.md)
