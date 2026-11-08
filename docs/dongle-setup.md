# Setting up zmk-led-feedback with a dongle

This guide covers the steps for using the `zmk-led-feedback` module for a ZMK dongle.

Important: before adding and editing files, create a branch in your `zmk-config` repository and push it to
GitHub. This keeps the dongle changes separate from your working configuration.

Run these commands from the root of your `zmk-config` repository to create a `dongle` branch:

```sh
git checkout -b dongle
git push -u origin dongle
```

First install this module by adding it to the `config/west.yml` file of your `zmk-config`. See the [Installation instructions](../README.md#installation)

Then set up the dongle as the central for your keyboard by following the official
[ZMK dongle guide](https://zmk.dev/docs/hardware-integration/dongle). That guide explains how to
create the dongle shield, make the keyboard halves peripherals, and build the firmware.

From here on out, the setup instructions are specific to this module. The split keyboard used in the examples is called `my_keyboard`. You should replace that with your own keyboard name.

## ZMK Config file structure

After setting up the dongle with this guide your zmk-config repository should look similar to this

```
├── boards/
│   └── shields/
│       └── my_keyboard/
│           ├── Kconfig.defconfig
│           ├── Kconfig.shield
│           ├── my_keyboard-layouts.dtsi
│           ├── my_keyboard.dtsi
│           ├── my_keyboard_sensors.dtsi    <- optional for encoders
│           ├── my_keyboard_dongle.overlay
│           ├── my_keyboard_left.overlay
│           ├── my_keyboard_right.overlay
│           └── ...
├── config/
│   ├── my_keyboard.conf
│   ├── my_keyboard_dongle.conf
│   ├── my_keyboard.keymap
│   └── west.yml
│
├── zephyr/
│   └── module.yml
│
└── build.yaml
```

## The build.yaml file

This is an example of what a `build.yaml` file could look like for a keyboard with a dongle setup using the `zmk-led-feedback` module. The halves are built as peripherals and the dongle is the central.

```yaml
include:
    - board: nice_nano_v2
      shield: my_keyboard_left
      cmake-args: -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
    - board: nice_nano_v2
      shield: my_keyboard_right
      cmake-args: -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
    - board: nice_nano_v2
      shield: settings_reset
    - board: nice_nano_v2
      shield: my_keyboard_dongle # <—- shield name for the dongle
      cmake-args: -DCONFIG_ZMK_DISPLAY=n
```

- The `cmake-args` on the halves make them peripherals, as described in the [ZMK dongle guide](https://zmk.dev/docs/hardware-integration/dongle#building-the-firmware).
- The dongle build sets `-DCONFIG_ZMK_DISPLAY=n` because the dongle has no display.
- See the [`settings reset documentation`](settings-reset.md) how to set it up if multiple boards are used for the dongle setup

## Dongle configuration file

ZMK loads a shield-specific `.conf` file for the dongle from the `config/` directory. Its name must match the dongle
shield name in the [`build.yaml`](#the-buildyaml-file) file. In this example, the dongle build uses the `my_keyboard_dongle` shield. So the dongle configuration file is: `config/my_keyboard_dongle.conf`.

Add that file in your `zmk-config` repository, as shown in the [example repository structure](#zmk-config-file-structure).

Add the LED strip driver options required by your hardware to this file:

```ini
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
```

Use the driver options appropriate to your LED strip. The example above is for a WS2812 strip using
SPI.

### Enable the LED feedback module

And finally enable this module by adding this in the same file.

```ini
# Enable the LED Feedback module
CONFIG_ZMK_LED_FEEDBACK=y
# The dongle has no display
CONFIG_ZMK_DISPLAY=n
```

All other module configuration options (you want to change) also go in this file.

You might have to copy other options over from your keyboard's shared `.conf` file to get everything working. For example, if you have [ `CONFIG_ZMK_POINTING`](https://zmk.dev/docs/features/pointing) in there you need to copy it to this file as well to have it working.

## The dongle overlay file

The dongle overlay file adds the dongle-specific hardware description for the firmware build.

Create an overlay file whose name matches the dongle shield. In this example, the shield is
`my_keyboard_dongle`, so the overlay file is `boards/shields/my_keyboard/my_keyboard_dongle.overlay`.

Add that file in your `zmk-config` repository, as shown in the [example repository structure](#zmk-config-file-structure).

### encoders

If your keyboard has encoders and their definitions are in a shared `.dtsi` file, move them into a separate file. Include that file back in the original shared `.dtsi` file, but also include it at the top of this file.

As you see in the [example repository structure](#zmk-config-file-structure) example the file for encoders is called `my_keyboard_sensors.dtsi`.

```dts
#include "my_keyboard_sensors.dtsi"
```

### Layouts

Do the same for physical layout definitions. Put them into a separate file, such as `my_keyboard-layouts.dtsi`, and include it from both the keyboard's shared `.dtsi` file and the dongle overlay. See the [ZMK dongle guide](https://zmk.dev/docs/hardware-integration/dongle#physical-layout).

### Example

The example below is the same as [adding RGB Underglow](https://zmk.dev/docs/hardware-integration/lighting/underglow#nrf52-based-boards) for nRF52-Based Boards in ZMK. The only difference being the mock kscan as described in the [ZMK dongle guide](https://zmk.dev/docs/hardware-integration/dongle#kscan) and the defined LED strip selected for LED feedback with `zmk,led-feedback = &led_strip;`.

This example shows a WS2812 strip with four LEDs, driven over SPI on pin P0.06. It was tested
with a [SuperMini NRF52840](https://github.com/joric/nrfmicro/wiki/Alternatives#supermini-nrf52840) dongle.

> [!WARNING]
> This example is hardware-specific. Before building any firmware with it, verify that the SPI controller (`&spi3`) and pin configuration (pin P0.06) match your board and LED wiring.

```dts
    #include <dt-bindings/led/led.h>

    // Include the file with encoder definitions here
    // Include the file with physical layout definitions here

    / {
        chosen {
            zmk,kscan = &mock_kscan;
            zmk,led-feedback = &led_strip;
        };

        // The dongle has no switches, the keys are sent by the two halves.
        mock_kscan: kscan_1 {
            compatible = "zmk,kscan-mock";
            columns = <0>;
            rows = <0>;
            events = <0>;
        };
    };

    &pinctrl {
        spi3_default: spi3_default {
            group1 {
                psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
            };
        };

        spi3_sleep: spi3_sleep {
            group1 {
                psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
                low-power-enable;
            };
        };
    };

    &spi3 {
        compatible = "nordic,nrf-spim";
        status = "okay";
        pinctrl-0 = <&spi3_default>;
        pinctrl-1 = <&spi3_sleep>;
        pinctrl-names = "default", "sleep";

        led_strip: ws2812@0 {
            compatible = "worldsemi,ws2812-spi";
            reg = <0>;
            spi-max-frequency = <4000000>;
            chain-length = <4>;
            spi-one-frame = <0x70>;
            spi-zero-frame = <0x40>;
            color-mapping = <LED_COLOR_ID_GREEN LED_COLOR_ID_RED LED_COLOR_ID_BLUE>;
        };
    };
```

Set `chain-length` to the number of LEDs in your strip.

## flashing and pairing

To build the dongle (and keyboard) firmware push the changes to GitHub. [Download the firmware](https://zmk.dev/docs/user-setup#download-the-archive) from the Actions tab as you would normally do. To flash the firmware and pair your split keyboard to the dongle follow the [flashing and pairing guide](flashing-and-pairing.md).
