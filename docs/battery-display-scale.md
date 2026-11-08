# Battery display scale

The battery feedback lights a percentage of the LEDs from the LED strip depending on the battery status (with a minimum
of one). The color depends on the battery percentage:

- red below 20%
- yellow from 20% through 60%
- green above 60%

A battery reading of 0% is treated as no reading and shows one blue LED.
These are the default colors; they can be changed with [`CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`](battery-feedback.md#battery-display-colors-and-scale).

The tables below show the displayed battery percentage ranges for strips with 1 to 10 LEDs. Each
range is inclusive. “LEDs” is the number lit for that range.

## 1 LED

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–19%           | Red    | 1    |
| 20–60%          | Yellow | 1    |
| 61–100%         | Green  | 1    |

## 2 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–19%           | Red    | 1    |
| 20–50%          | Yellow | 1    |
| 51–60%          | Yellow | 2    |
| 61–100%         | Green  | 2    |

## 3 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–19%           | Red    | 1    |
| 20–33%          | Yellow | 1    |
| 34–60%          | Yellow | 2    |
| 61–66%          | Green  | 2    |
| 67–100%         | Green  | 3    |

## 4 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–19%           | Red    | 1    |
| 20–25%          | Yellow | 1    |
| 26–50%          | Yellow | 2    |
| 51–60%          | Yellow | 3    |
| 61–75%          | Green  | 3    |
| 76–100%         | Green  | 4    |

## 5 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–19%           | Red    | 1    |
| 20%             | Yellow | 1    |
| 21–40%          | Yellow | 2    |
| 41–60%          | Yellow | 3    |
| 61–80%          | Green  | 4    |
| 81–100%         | Green  | 5    |

## 6 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–16%           | Red    | 1    |
| 17–19%          | Red    | 2    |
| 20–33%          | Yellow | 2    |
| 34–50%          | Yellow | 3    |
| 51–60%          | Yellow | 4    |
| 61–66%          | Green  | 4    |
| 67–83%          | Green  | 5    |
| 84–100%         | Green  | 6    |

## 7 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–14%           | Red    | 1    |
| 15–19%          | Red    | 2    |
| 20–28%          | Yellow | 2    |
| 29–42%          | Yellow | 3    |
| 43–57%          | Yellow | 4    |
| 58–60%          | Yellow | 5    |
| 61–71%          | Green  | 5    |
| 72–85%          | Green  | 6    |
| 86–100%         | Green  | 7    |

## 8 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–12%           | Red    | 1    |
| 13–19%          | Red    | 2    |
| 20–25%          | Yellow | 2    |
| 26–37%          | Yellow | 3    |
| 38–50%          | Yellow | 4    |
| 51–60%          | Yellow | 5    |
| 61–62%          | Green  | 5    |
| 63–75%          | Green  | 6    |
| 76–87%          | Green  | 7    |
| 88–100%         | Green  | 8    |

## 9 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–11%           | Red    | 1    |
| 12–19%          | Red    | 2    |
| 20–22%          | Yellow | 2    |
| 23–33%          | Yellow | 3    |
| 34–44%          | Yellow | 4    |
| 45–55%          | Yellow | 5    |
| 56–60%          | Yellow | 6    |
| 61–66%          | Green  | 6    |
| 67–77%          | Green  | 7    |
| 78–88%          | Green  | 8    |
| 89–100%         | Green  | 9    |

## 10 LEDs

| Battery         | Color  | LEDs |
| --------------- | ------ | ---- |
| 0% (no reading) | Blue   | 1    |
| 1–10%           | Red    | 1    |
| 11–19%          | Red    | 2    |
| 20%             | Yellow | 2    |
| 21–30%          | Yellow | 3    |
| 31–40%          | Yellow | 4    |
| 41–50%          | Yellow | 5    |
| 51–60%          | Yellow | 6    |
| 61–70%          | Green  | 7    |
| 71–80%          | Green  | 8    |
| 81–90%          | Green  | 9    |
| 91–100%         | Green  | 10   |
