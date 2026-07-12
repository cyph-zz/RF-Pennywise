# RF-Pennywise

A multi-radio ESP32 toolkit for 2.4GHz RF work — jamming, scanning, spectrum analysis, and WiFi/BLE security-awareness demos. Built on an ESP32-WROOM-32 with three nRF24L01+ modules, controlled with three buttons and a 128x64 OLED.

Forked from [RF-Clown](https://github.com/cifertech/rfclown) by CiferTech, rewritten and substantially extended.

> **Personal lab / educational use.** Several pages emit RF or run active WiFi/BLE features (jamming, beacon/BLE spam, honeypot). Only use them on your own equipment or with explicit authorization — see [Legal & Ethics](#legal--ethics) below.

## Hardware

- ESP32-WROOM-32
- 3x nRF24L01+ (PA+LNA recommended for range)
- 128x64 I2C OLED (SSD1306)
- WS2812 NeoPixel (activity indicator)
- 3 push buttons (left / right / center)

| Function | Pin |
|---|---|
| Button Left | GPIO 27 |
| Button Right | GPIO 25 |
| Button Center | GPIO 26 |
| nRF24 A — CE / CSN | GPIO 5 / 17 |
| nRF24 B — CE / CSN | GPIO 16 / 4 |
| nRF24 C — CE / CSN | GPIO 15 / 2 |

## Navigation

The menu has two levels:

1. **Category screen** — left/right cycles **JAM**, **SCAN**, **SPECTRUM**, **TOOLS**; center enters the category on its first page.
2. **Card screen** — left/right moves between pages in the current category; center toggles the page on/off (or opens a page-specific sub-screen). Left on the first card of a category returns to the category screen; right on the last card does nothing (no wraparound).

In **SCAN ONLY** device mode (see [System](#tools)), the entire **JAM** category is hidden from navigation and all RF-emitting pages are blocked from activating — this is the compliance switch for using the device without transmitting anything.

## Fixes to the original RF-Clown base

- Debounce was shared across all three buttons (one press could block the others) — split into independent debounce state per button.
- `neopixelLoop()` had an empty body and `neopixelSetup()` was never called — the activity LED never worked. Fixed the init path and wired the LED to real state.
- Radio deactivation left the nRF24 modules in an inconsistent state (auto ack, retries, CE pin) — `setRadiosNeutralState()` now fully powers down and neutralizes all three radios.
- Multiple-definition linker errors from globals declared in headers — moved real definitions into `setting.cpp`, headers only `extern`-declare.
- WiFi and Zigbee jam channel tables used the protocol's channel *numbers* directly as raw nRF24 channel offsets, landing on the wrong physical frequency. Recomputed both tables from the real WiFi/Zigbee channel-to-frequency formulas.

## Features

### JAM
Continuous-carrier jamming on the nRF24 radios, one page per target: WiFi, Video TX, RC, BLE, Bluetooth Classic, USB Wireless, Zigbee, NRF24. Plus two WiFi/BLE "spam" pages that don't jam but flood the air with fake advertisements:

- **Beacon Spam** — broadcasts a rotating set of fake WiFi SSIDs, one physical channel at a time (long enough on-air to actually show up in a phone's scan).
- **BLE Spam** — alternates Apple Continuity ("AirPods pairing popup") and Microsoft Swift Pair beacons to demonstrate BLE advertisement spoofing.

All eight jam pages are blocked and hidden in SCAN ONLY mode.

### SCAN
- **WiFi / BT / BLE Scan** — live scan results, paginated, with WPS detection flagged on the WiFi list.
- **Probe Sniff** — passive WiFi promiscuous-mode sniffer, logs devices sending probe requests (MAC, SSID requested, RSSI), sorted by signal strength with automatic expiry of stale entries.
- **Traffic Mon** — promiscuous-mode packet counter (management / control / data / misc) with live packets/sec on a selectable channel.

### SPECTRUM
- **BLE / WiFi Spectrum** — real-time RSSI bar graphs across the relevant band.
- **AIO Spectrum** — full 2.4GHz band energy scan with a selectable protocol overlay (All / WiFi / BT / BLE / Zigbee) highlighting which bars fall on real channels for that protocol.
- **Spy Locate** — single fixed-channel signal-strength meter ("fox hunt" style) for physically tracking down a transmitter, with a live protocol guess for the selected channel.

### TOOLS
- **System** — radio status, CPU load (via FreeRTOS idle hook sampling), OLED brightness, and the **JAM+SCAN / SCAN ONLY** device mode switch.
- **BrickWise** — a small Breakout/Arkanoid-style game, because why not.
- **Honeypot** — a fake open WiFi AP with a captive-portal login page, for demonstrating phishing risk. Configurable AP name (preset list or a built-in character-by-character editor), history of captured login/password pairs, paginated on-screen.

## Building

```
pio run              # compile
pio run -t upload    # flash
```

Requires [PlatformIO](https://platformio.org/). Dependencies (`U8g2`, `RF24`, `Adafruit NeoPixel`) are pulled automatically via `platformio.ini`. The project needs the `huge_app` partition scheme (already set) since BluetoothSerial + WiFi + BLE together overflow the default OTA partition layout.

## Legal & Ethics

This device can emit RF (nRF24 jamming) and run active WiFi/BLE attacks (beacon spam, BLE spam, honeypot). Depending on your jurisdiction, intentional jamming and unauthorized access-point spoofing are regulated or illegal. Use SCAN ONLY mode by default; only enable JAM+SCAN mode and the spam/honeypot pages on equipment you own or with explicit written authorization (pentest engagement, CTF, personal lab). The honeypot's password capture exists purely as a phishing-awareness demonstration — nothing is exported off the device (no SD card, no network callback).

## License

MIT — see [LICENSE](LICENSE). Retains the original copyright notice from [RF-Clown](https://github.com/cifertech/rfclown) (CiferTech, 2022).

---

MIT © 2026 [Cypher](https://github.com/cyph-zz) — [RF-Pennywise](https://github.com/cyph-zz/RF-Pennywise)
