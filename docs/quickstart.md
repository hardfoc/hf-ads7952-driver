# Quick Start

Get the ADS7952 driver reading ADC channels in under 20 lines.

---

## Prerequisites

- Library installed or added as a submodule (see [Installation](installation.md))
- SPI bus configured and wired (see [Hardware Setup](hardware_setup.md))
- A platform-specific SPI implementation (see [Platform Integration](platform_integration.md))

---

## Minimal Example

```cpp
#include "ads7952.hpp"
#include "my_platform_spi.hpp"  // Your CRTP SPI implementation

// Create SPI interface and driver (Vref = 2.5V, VA = 5.0V)
MyPlatformSpi spi(/* SPI bus config */);
ads7952::ADS7952<MyPlatformSpi> adc(spi, 2.5f, 5.0f);

// Initialize: idempotent — discards first invalid conversion, programs defaults
adc.EnsureInitialized();

// Read a single channel (returns structured result)
auto result = adc.ReadChannel(0);
if (result.ok()) {
    printf("CH0: %u counts (%.3f V)\n", result.count, result.voltage);
}
```

> ☣️ **Important:** Always call `EnsureInitialized()` before reading. The ADS7952's first conversion after power-up is invalid — this is handled automatically. Subsequent calls are no-ops.

---

## Step-by-Step

### 1. Implement the SPI Interface

Create a class that inherits from `ads7952::SpiInterface<YourClass>`. The only requirement is a `transfer()` method that performs a full-duplex SPI transaction:

```cpp
#include "ads7952_spi_interface.hpp"

class MyPlatformSpi : public ads7952::SpiInterface<MyPlatformSpi> {
    friend class ads7952::SpiInterface<MyPlatformSpi>;

protected:
    void transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
        // Your platform-specific SPI transfer (always 2 bytes for ADS7952)
    }
};
```

See [Platform Integration](platform_integration.md) for complete ESP32, STM32, and Linux examples.

### 2. Create and Initialize the Driver

```cpp
#include "ads7952.hpp"

MyPlatformSpi spi;
ads7952::ADS7952<MyPlatformSpi> adc(spi, 2.5f, 5.0f);  // Vref, VA

if (!adc.EnsureInitialized()) {
    // Handle initialization failure
}
```

The constructor takes three parameters:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `spi` | — | Your CRTP SPI interface instance |
| `vref` | 2.5 V | External reference voltage (REFP pin) |
| `va` | 5.0 V | Analog supply voltage (VA pin) |

`EnsureInitialized()` on first call performs:
- Discards the first (invalid) conversion after power-up
- Programs Auto-1 channel mask (default: all 12 channels)
- Programs Auto-2 last channel (default: CH11)
- Enters Auto-1 mode

Subsequent calls return `true` immediately without SPI traffic. Pass `force = true` to re-initialize after a device reset.

### 3. Read a Single Channel

`ReadChannel()` switches to Manual mode, sends a 2-frame SPI sequence, and returns a `ReadResult`:

```cpp
auto result = adc.ReadChannel(3);  // Read channel 3

if (result.ok()) {
    printf("Channel: %u\n", result.channel);   // 3
    printf("Count:   %u\n", result.count);     // 0–4095
    printf("Voltage: %.3f V\n", result.voltage); // auto-converted
} else {
    printf("Error: %u\n", static_cast<unsigned>(result.error));
}
```

### 4. Read All Channels (Batch)

`ReadAllChannels()` uses Auto-1 mode to sweep through all programmed channels in a single burst:

```cpp
auto all = adc.ReadAllChannels();

if (all.ok()) {
    for (uint8_t ch = 0; ch < 12; ch++) {
        if (all.hasChannel(ch)) {
            printf("CH%02u: %4u counts  (%.3f V)\n",
                   ch, all.count[ch], all.voltage[ch]);
        }
    }
}
```

### 5. Switch Operating Modes

```cpp
// Auto-1: sequence through a programmable channel mask
adc.ProgramAuto1Channels(ads7952::kFirstFour);  // CH0–CH3 only
adc.EnterAuto1Mode(true);          // true = reset channel counter

// Auto-2: sequential scan CH0 through last channel
adc.ProgramAuto2LastChannel(5);    // Scan CH0–CH5
adc.EnterAuto2Mode(true);

// Manual: explicit channel selection per read
auto r = adc.ReadChannel(7);       // ReadChannel auto-enters Manual mode
```

### 6. Change Input Range

```cpp
// Default: 0 to Vref (2.5V)
adc.SetRange(ads7952::Range::TwoVref);  // Now 0 to 2×Vref (5.0V)

auto r = adc.ReadChannel(0);
// r.voltage is now calculated against the 2×Vref reference
```

> 💡 When range is set to `TwoVref`, the active reference is clamped to `min(2×Vref, VA)` to prevent readings from exceeding the supply rail.

---

## What's Next?

- **[Hardware Setup](hardware_setup.md)** — Wiring, decoupling, reference selection
- **[Platform Integration](platform_integration.md)** — Complete CRTP SPI implementation guide
- **[Configuration](configuration.md)** — Kconfig, compile-time defaults, advanced tuning
- **[Examples](examples.md)** — Multi-mode, alarm thresholds, GPIO control

---

**Navigation**
⬅️ [Installation](installation.md)  ➡️ [Hardware Setup](hardware_setup.md)
