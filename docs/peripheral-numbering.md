# Peripheral numbering

This page explains pairing slots, mapped device numbers, and how to identify a peripheral.

## Device numbers and pairing slots

The central (dongle) assigns each peripheral a pairing slot when it first pairs. By default, device
numbers follow pairing-slot order: left half (device 1), right half (device 2), then a macropad
(device 3), for example. `CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` can map these slots to different
device numbers without changing the saved pairings.

The central remembers pairings and slots across normal firmware updates and disconnects. Device
numbers are the mapped numbers used by connection warnings and battery display keys.

- [Connection feedback](connection-feedback.md) uses pairing slot numbers to identify a missing peripheral
- [Battery feedback](battery-feedback.md) uses pairing slot numbers to select which peripheral's battery status to display.

For more information, see the [flashing and pairing guide](flashing-and-pairing.md).

## Changing the pairing order

> [!NOTE]
> To avoid confusion, consider [resetting the pairing order](flashing-and-pairing.md#flashing-firmware-and-pairing-for-the-first-time) instead of changing the mapping.

To assign different device numbers to existing pairing slots without pairing them again, use the
`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

For example, if the right half paired first and should be device 2, while the left
half paired second and should be device 1, use:

```ini
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1"
```

Here, the first value (`2`) maps the first pairing slot (the right half) to device number 2, and
the second value (`1`) maps the second slot (the left half) to device number 1. A nonempty mapping
must contain exactly one occurrence of every number from `1` through the configured peripheral
count. An empty value uses identity order; malformed, duplicate, missing, extra, or out-of-range
entries fail the build.

`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` selects how many mapped device numbers, starting
from 1, are required for both connection and battery warnings. Its default `-1` means all slots;
`0` means none; values `1..N` require those device numbers. The value cannot exceed the configured
slot count. Optional peripherals remain available to on-demand battery display. See
[connection feedback](connection-feedback.md#required-peripherals).

## Identifying a device number

To find a peripheral's [device number](#device-numbers-and-pairing-slots), keep the other
peripherals connected and power off the one you want to identify during the boot warning window.
If connection warnings are enabled and this device is required, the warning uses its mapped device
number when the strip can distinguish the configured slots. For example, two blinking LEDs
identify device 2.

This only works when the LED strip has at least one LED per configured peripheral. If the strip is
shorter, [connection feedback](connection-feedback.md) uses a generic warning and doesn’t identify individual peripherals.
