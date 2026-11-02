"""The three layers of the driver: hardware-agnostic core, SPI interface, platform port.

Reads: From top to bottom. Your application talks to one ADS7952<SpiType> object, whose public calls (listed from
inc/ads7952.hpp) read channels, switch modes and program sequences, alarms and GPIOs. The core builds the 16-bit
command words, keys every result on the channel address in DO15:12 and converts counts to volts. It reaches the bus
through the CRTP SpiInterface (inc/ads7952_spi_interface.hpp), whose one method the platform port implements: a
full-duplex transfer with chip select held low. Below it is the hardware.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/ads7952.hpp"
SPI = "inc/ads7952_spi_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    public = drv.split("class ADS7952 {", 1)[1].split("private:", 1)[0]
    calls = []
    for name in re.findall(r"^  (?:static constexpr |explicit )?[\w:&<>]+\s+(\w+)\(", public, re.M):
        if name == "ADS7952" or name.startswith(("Get", "Is")) or name in calls:
            continue
        calls.append(name)
    spi = (ROOT / SPI).read_text()
    hooks = re.findall(r"^  (?:bool|void) (\w+)\(const uint8_t", spi, re.M)
    return {
        "calls": calls,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:ADS7952 public calls", f"{SPI}:SpiInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic ADS7952 driver, which builds 16-bit command "
        "frames and calls the CRTP SPI interface, which calls the one transfer method of the platform port you write, "
        "which drives the SPI bus of the ADS7952.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a class with one method.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["sensor polling, control loop, CLI ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, [c + "()" for c in d["calls"]], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="ADS7952<SpiType>, inc/ads7952.hpp")
    comm = f.card(0, reg.b + 40, W, "SPI interface",
                  ["SpiInterface<Derived> (CRTP): 16-bit frames, MSB first, one transfer per frame,",
                   "results keyed on the channel address in DO15:12, no heap, no virtual calls"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)",
                  ["  ".join(h + "(tx, rx, len)" for h in d["port_hooks"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "ADS7952", ["SPI mode 0 up to 20 MHz  /  CS, SCLK, SDI, SDO  /  12 inputs, 4 GPIOs"],
                tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
