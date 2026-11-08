# Flashing and pairing

This guide explains how to flash and pair a ZMK split keyboard that uses a dongle as its central.
Before you begin, it's assumed you've completed [the dongle setup](dongle-setup.md) and build the firmware for all devices.

You should now have firmware files for:

- [`settings_reset`](settings-reset.md) (to clear saved settings and pairings)
- the left half
- the right half
- the dongle

## Flashing firmware and pairing for the first time

Use these instructions for pairing for the first time, or to reset the [pairing order](peripheral-numbering.md#device-numbers-and-pairing-slots)

1. Flash a [`settings_reset`](settings-reset.md) to every device that participates in the setup (including the dongle). This will clear all saved settings on the micro controllers.
2. Flash the firmware for the left and right halves of the keyboard first. After flashing, power the devices off. Flash the firmware for the dongle last.
3. Power on the dongle (by connecting to usb). You probably see red leds blinking, warning you the keyboard halves are not connected yet. Then pair the keyboard halves to the dongle one at a time, in the order you want them assigned to a [pairing slot](#pairing-slots). Wait for each peripheral to connect before powering on the next.

## Updating firmware after pairing

After the initial setup, you do not need to flash a `settings_reset` again for a normal firmware
update. Flash the device whose firmware contains the change:

- For keymap changes, such as bindings, layers, combos, or encoder actions, flash only the dongle.
- For changes to a half's hardware configuration, such as its matrix wiring or encoder pins, flash
  that half.
- For dongle-specific changes, such as LED feedback configuration, flash the dongle.
- If another configuration change affects multiple devices, flash each affected device.

The dongle remembers the [peripheral pairings and their slots](#pairing-slots) across normal firmware updates, so reflashing does not normally change the slot order. Use a `settings_reset` when you want to erase the saved pairings; then pair the halves again in the desired order.

## Pairing slots

When a peripheral first pairs with the dongle, it is assigned a pairing slot. The dongle keeps
that assignment when the dongle, or any of the peripherals, disconnects and reconnects.

Connection feedback uses these slot numbers (also known as [device numbers](peripheral-numbering.md#device-numbers-and-pairing-slots)) to identify a missing peripheral, and
battery feedback uses them to select which peripheral's battery to display.

You can see the pairing slot by powering off a keyboard half. With enough LEDs, [connection feedback](connection-feedback.md#connection-warning) will show a warning by blinking red leds. The number of blinking LEDs match the missing peripheral's device number (pairing slot).

## Connecting the dongle to your computer

After the peripherals are paired with the dongle, connect the dongle to your computer using either:

- **USB:** Connect it with a USB data cable. The computer receives keypresses over USB.
- **Bluetooth:** The dongle can be powered by a charger while using Bluetooth to connect to your computer. Select a host profile using a keymap binding such as [`&bt BT_SEL 0`](https://zmk.dev/docs/keymaps/behaviors/bluetooth), then pair the dongle from the computer's Bluetooth settings. It appears using the configured keyboard name. If the dongle's microprocessor was previously paired (to your computer) under a different name, remove that old entry from the computer before pairing again.
