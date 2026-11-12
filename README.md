# zmk-led-feedback

> [!WARNING]
> This project is undergoing changes. Backward compatibility is not currently maintained;
> updates may require changes to your configuration.

A [ZMK](https://zmk.dev) module that shows the state of your keyboard on a small WS2812-style LED strip. It runs on the central, which makes it a good fit for a [dongle](https://zmk.dev/docs/hardware-integration/dongle). Use it with a single LED or as many as you want.

When you add a dongle to your (split) keyboard setup it takes on the role of "**central**". All other parts of the keyboard are called "**peripherals**". For more information see ZMK's [central and peripheral roles](https://zmk.dev/docs/features/split-keyboards#central-and-peripheral-roles). In the rest of the documentation you can assume "the central" means "the dongle"

This module can also be used on regular (split) keyboards, but it only reports on the status of peripheral batteries. Using normal [RGB underglow](https://zmk.dev/docs/hardware-integration/lighting/underglow) is usually the better choice there.

## Features

- [Layers](#layer-feedback)
    - Shows the current active layer briefly on the LED strip after a layer change.
    - Or display the current active layer continuously.
    - Change layer colors and layer display types
    - Hooks for use in your own zmk-config repo to change module behavior
- [Connection Status](#connection-feedback)
    - Optionally warns when a required peripheral is missing during the first 20 seconds after module initialization.
- [Battery Status](#battery-feedback)
    - Shows the battery level of a peripheral (on request with a keycode).
    - Optionally warns once per boot when a required peripheral reports a nonzero battery level below a threshold.
- [Keycodes](docs/keycodes.md)
    - Change the features displayed on the dongle with keycodes.
- [Connection](docs/flashing-and-pairing.md#connecting-the-dongle-to-your-computer)
    - Connect the dongle with a USB cable or use Bluetooth (when the dongle is powered by a charger).

#### Warning behavior

Connection and low-battery warnings are independent opt-ins and both default off. When enabled,
they only detect eligible events during the 20 seconds after module initialization. Connection
warnings take priority over battery warnings; a pending battery warning runs after the connection
warning ends and may continue past the boot window. A connection red phase starts only when its
full configured interval fits before the boot deadline; connection warnings never spill past boot.
Warnings take priority over layer feedback, on-demand battery displays,
and brightness previews. Turning the display off suppresses all output and cancels an already-
started battery-warning sequence. See the [connection](docs/connection-feedback.md) and
[battery](docs/battery-feedback.md#low-battery-warnings) guides for details.

## Compatibility

Built for ZMK v0.3 and nRF52840 (nice!nano v2).</br>

### Keyboards

The dongle is designed to work with ZMK v0.3 (split) keyboards built with BLE-capable controller boards listed in [ZMK’s supported hardware](https://v0-3-branch.zmk.dev/docs/hardware). But you'll have to configure the keyboard controllers as peripherals to pair them with the dongle.

### Dongle

Use a BLE-capable controller board listed in [ZMK’s v0.3 supported hardware](https://v0-3-branch.zmk.dev/docs/hardware) and configure it as the central. The board must also support a LED-strip driver compatible with your strip. See the [dongle setup guide](docs/dongle-setup.md).

### Tested controller boards

- [supermini-nrf52840](https://github.com/joric/nrfmicro/wiki/Alternatives#supermini-nrf52840) (nicenano-v2 clone) controller board for the dongle.

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
- [Peripheral numbering documentation](docs/peripheral-numbering.md)

## Configuration Overview

### General

These options are shared between this module's feedback features. Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle.

#### Configuration options

| Option                                                                           | Default | Description                                                                                                |
| -------------------------------------------------------------------------------- | ------- | ---------------------------------------------------------------------------------------------------------- |
| [`CONFIG_ZMK_LED_FEEDBACK`](docs/dongle-setup.md#enable-the-led-feedback-module) | `n`     | Enable this module.                                                                                        |
| `CONFIG_ZMK_LED_FEEDBACK_REVERSE`                                                | `n`     | Start from the end of the LED strip                                                                        |
| `CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS`                                             | `64`    | Initial brightness (1-255); must be between the configured minimum and maximum.                            |
| `CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS`                                         | `10`    | Minimum brightness (1-255). Runtime brightness keys stop at this limit. (See [Keycodes](docs/keycodes.md)) |
| `CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS`                                         | `128`   | Maximum brightness (1-255). Runtime brightness keys stop at this limit. (See [Keycodes](docs/keycodes.md)) |
| `CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP`                                        | `16`    | Brightness change for each key press.                                                                      |
| `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`                                      | `500`   | Warning phase duration in milliseconds; accepted range is `100-1000`.                                      |

Brightness values must satisfy `minimum <= initial <= maximum`; invalid ranges or ordering fail
the build instead of silently clamping the initial value.

##### Example

```ini
# Enable the module
CONFIG_ZMK_LED_FEEDBACK=y
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
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT` | `0`               | With per-layer colors: `0` cycles, `-1` lights all LEDs, or `1..strip length` lights a fixed count.     |
| `CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON`                  | `n`               | Keep showing the active layer. Can be toggled with [`LF_LAYER_TOG`](docs/keycodes.md#general-keycodes). |
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

| Option                                         | Default | Description                                                                                |
| ---------------------------------------------- | ------- | ------------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`           | `n`     | Enable boot-only connection warnings.                                                      |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | Shared warning filter: `-1` means all, `0` none, or `1..N` mapped device numbers required. |

### Battery feedback

Battery feedback can display peripheral battery levels on request and optionally warn when a
peripheral battery is low.

This feature is not enabled by default. Enable it in the `.conf` file for the central:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
```

Find out more about battery feedback in the [battery feedback documentation](docs/battery-feedback.md).

- [Configuration](docs/battery-feedback.md#configuration)
- [keycodes](docs/battery-feedback.md#keycodes)
- [Enabling battery feedback](docs/battery-feedback.md#enabling-battery-feedback)
- [Battery display colors and scale](docs/battery-feedback.md#battery-display-colors-and-scale)
- [Low-battery warnings](docs/battery-feedback.md#low-battery-warnings)

For percentage-to-LED mappings, see [Battery display scale](docs/battery-display-scale.md).

#### Configuration options

Use them in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle:

| Option                                           | Default                         | Description                                                            |
| ------------------------------------------------ | ------------------------------- | ---------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY`                | `n`                             | Enable the `led_bat` behavior and peripheral battery-level fetching.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`         | `"00FF00,FFFF00,FF0000,0000FF"` | Battery colors in order: high, medium, low, no reading.                |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`  | `0`                             | Boot warning threshold: `0` off, or `2-100`; level `0` is unavailable. |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS` | `5`                             | Battery warning cycles (`1-10`), completed at most once per boot.      |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS`       | `500`                           | Off time after a battery key press before the first level appears.     |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS`    | `1000`                          | How long each device's battery level stays lit.                        |

### Peripheral Configuration

When a peripheral first pairs with the dongle, it is assigned a [pairing slot number](docs/peripheral-numbering.md#device-numbers-and-pairing-slots). The dongle keeps that assignment when the dongle or any of the peripherals disconnects and reconnects.

This option allows you to change the device-number mapping without re-pairing peripherals to the dongle. Use it in your zmk-config [`.conf`](docs/dongle-setup.md#dongle-configuration-file) file for the dongle.

| Option                               | Default | Description                                                                                        |
| ------------------------------------ | ------- | -------------------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` | `""`    | Maps pairing slots to device numbers for warnings and battery display; empty means identity order. |

For an example of this option see [peripheral numbering documentation](docs/peripheral-numbering.md#changing-the-pairing-order)

## License

[MIT](LICENSE)
