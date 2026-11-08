# zmk-led-feedback

A [ZMK](https://zmk.dev) module that shows the state of your keyboard on a small WS2812-style LED strip. It runs on the central, which makes it a good fit for a [dongle](https://zmk.dev/docs/hardware-integration/dongle). Use a single LED or as many as you need.

When you add a dongle to your (split) keyboard setup it takes on the role of "**central**". All other parts of the keyboard are called "**peripherals**". For more information see ZMK's [central and peripheral roles](https://zmk.dev/docs/features/split-keyboards#central-and-peripheral-roles). In the rest of the documentation you can assume "the central" means "the dongle"

This module can also be used on regular (split) keyboards, but it only reports on the status of peripheral batteries. Using normal [RGB underglow](https://zmk.dev/docs/hardware-integration/lighting/underglow) is usually the better choice there.

## Features

- [Layers](#layer-feedback)
    - Shows the current active layer briefly on the LED strip after a layer change.
    - Or display the current active layer continuously.
    - Change layer colors and layer display types
    - Hooks for use in your own zmk-config repo to change module behavior
- [Connection Status](#connection-feedback)
    - Blinks red LEDs while a required peripheral is not connected.
- [Battery Status](#battery-feedback)
    - Shows the battery level of a peripheral (on request with a keycode).
    - Option to show a warning if batteries go beneath a threshold.
- [Keycodes](docs/keycodes.md)
    - Change the features displayed on the dongle with keycodes.
- [Connection](docs/flashing-and-pairing.md#connecting-the-dongle-to-your-computer)
    - Connect the donge with a USB cable or use Bluetooth (when the dongle is powered by a charger).

#### Priority

The layer indication has priority over connection warnings. The warning resumes afterwards. A battery display (shown on request) temporarily takes priority over both. Layer changes stops the battery display.

## Build

Built for ZMK v0.3 and the nRF52840 (nice!nano v2).
Tested with a [supermini-nrf52840](https://github.com/joric/nrfmicro/wiki/Alternatives#supermini-nrf52840)

## Installation

Add this module to your zmk-config `config/west.yml` file:

<!-- prettier-ignore -->
```yaml
manifest:
    remotes:
        - name: zmkfirmware
          url-base: https://github.com/zmkfirmware
        - name: keesiemeijer                            # <--- add this
          url-base: https://github.com/keesiemeijer     # <--- add this
    projects:
        - name: zmk
          remote: zmkfirmware
          revision: v0.3 # <-- Only supported ZMK version for this module
          import: app/west.yml
        - name: zmk-led-feedback                        # <--- add this
          remote: keesiemeijer                          # <--- add this
          revision: main                                # <--- add this
    self:
        path: config
```

## Setting up this module with a dongle

See these instructions on setting up `zmk-led-feedback` with a dongle:

- [dongle setup guide](docs/dongle-setup.md)
- [flashing, pairing, and connecting to a computer](docs/flashing-and-pairing.md)
- [Keycodes documentation](docs/keycodes.md)
- [Peripheral numbering documentation](/docs/peripheral-numbering.md)

## Configuration Overview

### General

These options are shared between this module's feedback features. Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle.

#### Configuration options

| Option                                                                               | Default | Description                                                                                                                                                                             |
| ------------------------------------------------------------------------------------ | ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [`CONFIG_ZMK_LED_FEEDBACK`](docs/dongle-setup.md#enable-the-led-feedback-module)     | `n`     | Enable this module.                                                                                                                                                                     |
| [`CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF`](docs/display-off.md#configuration-options)   | `n`     | Start with the LED display off.                                                                                                                                                         |
| [`CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES`](docs/display-off.md#configuration-options) | `""`    | Comma-separated list of [feedback types](docs/display-off.md#override-feedback-types) allowed to show feedback temporarily while the display is off. Empty means no feedback overrides. |
| `CONFIG_ZMK_LED_FEEDBACK_REVERSE`                                                    | `n`     | Start from the end of the LED strip                                                                                                                                                     |
| `CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS`                                                 | `64`    | Initial brightness (1-255). Limited to the min and max brightness below.                                                                                                                |
| `CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS`                                             | `10`    | Minimum brightness (1-255). The brightness never goes below this, also with the brightness keys. (See [Keycodes](docs/keycodes.md))                                                     |
| `CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS`                                             | `128`   | Maximum brightness (1-255). The brightness never goes above this, also with the brightness keys. (See [Keycodes](docs/keycodes.md))                                                     |
| `CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP`                                            | `16`    | Brightness change for each key press.                                                                                                                                                   |
| `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`                                          | `500`   | Duration of connection and battery warning blinks.                                                                                                                                      |

Example options:

```ini
# Enable the module
CONFIG_ZMK_LED_FEEDBACK=y

# Allow warnings when the led display is off (with the DISPLAY_OFF option or LF_OFF keycode)
CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="battery_warning,connection_warning"
```

### Layer feedback

Layer feedback shows the current active layer and can optionally stay on continuously.

Find out more about layer feedback in the [Layer feedback documentation](docs/layer-feedback.md).

- [Configuration](docs/layer-feedback.md#configuration)
- [Keycodes](docs/layer-feedback.md#keycodes)
- [Layer colors](docs/layer-feedback.md#layer-colors)
- [Change LED count and color with Hooks](docs/layer-feedback.md#change-led-count-and-color-with-hooks)

#### Configuration options

Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle.

| Option                                                     | Default           | Description                                                                                             |
| ---------------------------------------------------------- | ----------------- | ------------------------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER`                            | `y`               | Enable active-layer indication.                                                                         |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS`                     | `"00FF00,FFFF00"` | Comma-separated `RRGGBB` colors for layer groups; each color is used until the strip fills.             |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER`           | `""`              | Optional per-layer color palette; when set, overrides `LAYER_COLORS`.                                   |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT` | `0`               | `0` Default. same as `COLORS_PER_LAYER`, `-1` lights all LEDs, `N` lights N LEDs.                       |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON`                  | `n`               | Keep showing the active layer. Can be toggled with a [keycode]().                                       |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS`           | `""`              | Comma-separated layer numbers (for example `"2,7"`) to keep showing even when `LAYER_ALWAYS_ON` is off. |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_DURATION_MS`                | `1000`            | How long to show the layer after a layer change.                                                        |

### Connection feedback

Connection feedback uses the LED strip to display a warning when a required peripheral is disconnected.

Find out more about connection feedback in the [connection feedback documentation](docs/connection-feedback.md).

- [Configuration](docs/connection-feedback.md#configuration)
- [Connection warning](docs/connection-feedback.md#connection-warning)
- [Required peripherals](docs/connection-feedback.md#required-peripherals)

#### Configuration options

Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle:

| Option                                                    | Default | Description                                                                               |
| --------------------------------------------------------- | ------- | ----------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`                      | `y`     | Enable connection warnings for disconnected peripherals.                                  |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` | `-1`    | Number of required peripherals (`-1` means all). Higher-numbered peripherals are ignored. |

### Battery feedback

Battery feedback can display peripheral battery levels on request and optionally warn when a
peripheral battery is low.

This feature is not enabled by default. Enable it in the `.conf` file for the central:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
```

Find out more about connection feedback in the [battery feedback documentation](docs/battery-feedback.md).

- [Configuration](docs/battery-feedback.md#configuration)
- [keycodes](docs/battery-feedback.md#keycodes)
- [Enabling battery feedback](docs/battery-feedback.md#enabling-battery-feedback)
- [Battery display colors and scale](docs/battery-feedback.md#battery-display-colors-and-scale)
- [Low-battery warnings](docs/battery-feedback.md#low-battery-warnings)

For percentage-to-LED mappings, see [Battery display scale](docs/battery-display-scale.md).

#### Configuration options

Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle:

| Option                                                   | Default                         | Description                                                          |
| -------------------------------------------------------- | ------------------------------- | -------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY`                        | `n`                             | Enable the `led_bat` behavior and peripheral battery-level fetching. |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`                 | `"00FF00,FFFF00,FF0000,0000FF"` | Battery colors in order: high, medium, low, no reading.              |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`          | `0`                             | Warn below this battery percentage; `0` disables warnings.           |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES` | `0`                             | Repeat interval while a battery remains low; `0` disables repeats.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS`         | `5`                             | Number of warning blinks (1–1000).                                   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS`               | `500`                           | Off time after a battery key press before the first level appears.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS`            | `1000`                          | How long each device's battery level stays lit.                      |

### Peripheral Configuration

When a peripheral first pairs with the dongle, it is assigned a [pairing slot number](docs/peripheral-numbering.md#device-numbers-and-pairing-slots). The dongle keeps that assignment when the dongle or any of the peripherals disconnects and reconnects.

This option allows you to change the pairing order without re-pairing peripherals to the dongle. Use it in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle.

| Option                               | Default | Description                                                                                        |
| ------------------------------------ | ------- | -------------------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` | `""`    | Maps peripherals from pairing order to device numbers for connection feedback and battery display. |

For an example of this option see [peripheral numbering documentation](docs/peripheral-numbering.md#changing-the-pairing-order)

## License

[MIT](LICENSE)
