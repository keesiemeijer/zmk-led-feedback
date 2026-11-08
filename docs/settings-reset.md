# Resetting ZMK settings

Use a ZMK [`settings_reset`](https://zmk.dev/docs/troubleshooting/connection-issues#building-a-reset-firmware) firmware when you need to clear saved settings and Bluetooth pairings. For example when [pairing a split keyboard for the first time](flashing-and-pairing.md#flashing-firmware-and-pairing-for-the-first-time).

## Build reset firmware for each board

Build a `settings_reset` target for every distinct board used in the dongle setup. The shield name is the same for each target. For example, if the dongle uses a nice!nano v2 and the peripherals use a Seeeduino XIAO BLE:

```yaml
include:
    - board: nice_nano_v2
      shield: settings_reset
      artifact-name: settings_reset_nice_nano
    - board: seeeduino_xiao_ble
      shield: settings_reset
      artifact-name: settings_reset_xiao_ble
```

The `artifact-name` values make the resulting firmware files easy to distinguish. Flash each reset image only to devices using its matching board.

If every device in the dongle setup uses the same board, one build target can be used for all of them.
