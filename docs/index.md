---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# ADS7952 Driver

<p>A C++20 driver for the Texas Instruments ADS7952 12-channel, 12-bit, 1 MSPS SAR ADC. It reads one channel or a
whole sequence, programs per-channel alarms and the four GPIOs, and converts counts to volts, on any MCU with SPI.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-ads7952-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that moves bytes over SPI; it does the rest: the 16-bit
command words, the two-frame result pipeline, the power-up discard, sequence and alarm programming and the
conversion to volts.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">12-channel, 12-bit, 1 MSPS SAR ADC</span>
<span class="hf-chip__name">ADS7952</span>
<span class="hf-chip__vendor">Texas Instruments</span>
</div>

Inputs
:   12 single-ended channels through one multiplexer (MXO, wired to AINP)

Interface
:   SPI mode 0 up to 20 MHz, 16-bit frames, MSB first

Input range
:   0 to VREF, or 0 to 2 × VREF while 2 × VREF ≤ +VA; external VREF 2.0 V to 3.0 V

Supply
:   +VA 2.7 V to 5.25 V (analog), +VBD 1.7 V to +VA (digital I/O)

Extras
:   Manual, Auto-1 and Auto-2 sequencing; high and low alarm per channel; 4 GPIOs (TSSOP)

Package
:   38-pin TSSOP (DBT) or 32-pin VQFN (RHB, one GPIO); −40 °C to 125 °C

[Datasheet (SLAS605C)](datasheet/ads7952.pdf) · [Product page](https://www.ti.com/product/ADS7952)
</div>

## What you get

- **One object per chip.** `ADS7952<YourSpi>` reads channels, switches modes and programs sequences, alarms and
  GPIOs. See [API guide](api-guide.md).
- **Results matched to their channel.** Every reply carries the 4-bit address of the channel it belongs to; the
  driver keys on it instead of counting frames. See [Modes and the frame pipeline](modes-and-pipeline.md).
- **Volts in and out.** Counts convert with the reference and range you run, and alarm thresholds take volts. See
  [Configuration](configuration.md).
- **Zero-overhead portability.** The port is a CRTP class with one method: no virtual calls, no heap, no RTOS
  dependency. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the chip's SPI pipeline. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the chip, write the port, read every channel in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, bus settings, timing, reference and supply ranges.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Configure, read, program alarms and GPIOs, handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the SPI interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
