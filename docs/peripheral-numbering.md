# Peripheral numbering

This page explains peripheral device numbers, slot order numbers, required peripherals, and how to identify a device by its number.

## Device numbers and pairing slots

The central (dongle) assigns each peripheral a pairing slot when it first pairs. By default, device numbers follow pairing-slot order numbers. Pair devices in a fixed order to the central, for example: left half (device 1), right half (device 2), then a macropad (device 3).

The central remembers the peripheral pairings and their slot numbers across normal firmware updates and disconnects.

- [Connection feedback](connection-feedback.md) uses pairing slot numbers to identify a missing peripheral
- [Battery feedback](battery-feedback.md) uses pairing slot numbers to select which peripheral's battery status to display.

For more information, see the [flashing and pairing guide](flashing-and-pairing.md).

## Required peripherals

Required peripherals trigger both connection and battery warnings. All paired peripherals are required by default. Use the shared `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` option to set the number of specific peripherals as required. Use this option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

| Value | Result                                         |
| ----- | ---------------------------------------------- |
| `-1`  | Default. All paired peripherals are required   |
| `0`   | No peripherals are required for either warning |
| `1`   | first paired device is required                |
| `N`   | N devices paired to the dongle are required    |

The required count cannot exceed the number of [pairing slots](#device-numbers-and-pairing-slots).

In this example, only device numbers 1 and 2 are required. Higher-numbered devices are optional
and do not trigger warnings:

```ini
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
```

## Changing the pairing order

> [!NOTE]
> To avoid confusion, consider [resetting the pairing order](flashing-and-pairing.md#flashing-firmware-and-pairing-for-the-first-time) instead of changing the mapping with this option.

To assign different device numbers to existing pairing slots without pairing them again, use the
`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

For example, if the right half paired first and should be device 2, while the left
half paired second and should be device 1, use:

```ini
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1"
```

Here, the first value (`2`) maps the first pairing slot (the right half) to device number 2, and
the second value (`1`) maps the second slot (the left half) to device number 1.

## Identifying a device number

To find a peripheral's [device number](#device-numbers-and-pairing-slots), keep the other
peripherals connected and power off the one you want to identify during the [boot warning window](../README.md#warning-behavior). If connection warnings are enabled and the device is required, the connection warning blinks red LEDs equal to its device number. For example, two blinking LEDs identify device 2.

This only works when the LED strip has at least one LED per configured peripheral. If the strip is
shorter, [connection feedback](connection-feedback.md) uses a generic warning and doesn’t identify individual peripherals.
