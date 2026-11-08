# Peripheral numbering

This page explains peripheral device numbers, slot order numbers and how to identify a device by its number.

## Device numbers and pairing slots

The central (dongle) identifies peripherals by the order in which they first paired and assigns each to a pairing slot number. Pair devices in a fixed order to the central, for example, left half (device 1), right half (device 2), and then a macropad (device 3). 

In this documentation, the `pairing slot` (number) is the same as the `device number`. The central remembers the peripheral pairings and their slot numbers across normal firmware updates and disconnects.

- [Connection feedback](connection-feedback.md) uses pairing slot numbers to identify a missing peripheral
- [Battery feedback](battery-feedback.md) uses pairing slot numbers to select which peripheral's battery status to display.

for more information see the [flashing and pairing guide](flashing-and-pairing.md)

## Changing the pairing order

> [!NOTE]
> To avoid confusion in the future, it's better to [reset the pairing order](flashing-and-pairing.md#flashing-firmware-and-pairing-for-the-first-time) than reshuffling the order with this option.

To give peripherals a different [pairing slot number](#device-numbers-and-pairing-slots) without pairing them again, use the
`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

For example, if the right half paired first and should be device 2, while the left
half paired second and should be device 1, use:

```ini
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1"
```

Here, the first value (`2`) applies to the first peripheral paired (the right half), and the
second value (`1`) applies to the second (the left half).

For details about connection warnings and optional peripherals, see
[Connection feedback](connection-feedback.md).

## Identifying a device number

To find a peripheral's [device number](#device-numbers-and-pairing-slots), keep the other peripherals connected and power off the one you want to identify. If it is [configured as required](connection-feedback.md#required-peripherals), the connection warning blinks red LEDs equal to its device number. For example, two blinking LEDs identify device 2.

This only works when the LED strip has at least one LED per configured peripheral. If the strip is
shorter, [connection feedback](connection-feedback.md) uses a generic warning and doesn’t identify individual peripherals.
