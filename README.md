# usbswitch-remote

Replacement firmware and optional wired remote for UGREEN brand USB 2.0 sharing switch.

I've tested this on a version of the switch from 2017 which had five USB type A sockets,
and the button was in one corner. Newer versions of the same product appear to have
four Type B and one Type A, which no longer violates the USB spec, and the button is
in the center. I'm not sure if those units use compatible firmware.

This device is effectively a high-speed analog multiplexer, based on a tree of
three [SV7030 USB 2.0 switch](http://product.savitech-ic.com/DataSheet/SV7030DS-REV095a.pdf)
chips controlled by an [STM8S003F3 microcontroller](https://www.st.com/resource/en/datasheet/stm8s003k3.pdf).

## Firmware

The STM8 is a low-cost 8-bit micro with 8kB of flash memory, and there's an available PCB footprint
for an STLink or compatible debug interface.

There is no GCC or LLVM port, but we can tolerate [SDCC](http://sdcc.sourceforge.net/) for a small project like this!

The included firmware is written in C for the bare metal, no vendor libraries are needed.
It supports switching channels using the built-in button or an optional wired remote.
The original firmware had quite bad button debouncing, which is fixed. I've also omitted
support for automatic switching with voltage sensing. It could be added, but I didn't want
the feature as I found it distracting.

* Build the firmware with [SDCC](http://sdcc.sourceforge.net/).
* Install it with either [OpenOCD](http://openocd.org/) or [stm8flash](https://github.com/vdudouyt/stm8flash)

## Remote

To have direct control over switching to each of the four channels, a simple wired
remote with four lighted buttons can be added using an audio cable and some passive
components. See the included schematic. It uses a variant of charlieplexing that drives
four LEDs as usual, then splits the two remaining currents paths into two buttons each,
using an RC circuit to disambiguate them. If this needs a name, I'm calling it Tucoplexing
after my cat Tuco! 😸

