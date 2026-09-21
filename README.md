# Deauther Watch v2.0 — ESP32-S3 SuperMini Smartwatch & BadUSB Multi-Tool

**Deauther Watch v2.0** is an advanced wearable wireless security research device and BadUSB HID payload injection tool built on the **ESP32-S3 SuperMini** board. 

It combines Wi-Fi security auditing, BLE/Bluetooth scanning and jamming, nRF24 2.4GHz spectrum testing, BadUSB execution with native **Turkish Q keyboard layout support**, thermal monitoring, and a real-time digital smartwatch interface into a single compact device.

---

## Key Features & Architecture

### BadUSB HID Engine (Turkish Q Keyboard Support)
- Native USB HID keyboard emulation using hardware CDC / TinyUSB stack.
- Full **Turkish Q character mapping** (handling special characters like `ğ`, `ü`, `ş`, `ı`, `ö`, `ç`, `İ`, `Ğ`, `Ü`, `Ş`, `Ö`, `Ç`).
- Automated built-in payloads:
  - **System Info:** Dumps OS version, hostname, architecture, and RAM in a Windows popup.
  - **Add Admin User:** Hidden admin user creation with registry stealth settings.
  - **Wi-Fi Password Extractor:** Extracts all stored Windows Wi-Fi profiles and cleartext passwords into a GUI popup.
  - **Defender Toggle:** Toggles Windows Defender real-time monitoring via PowerShell.
  - **AMSI Bypass:** Memory patch bypass for AMSI logging.
  - **Lock Screen:** Instant Windows workstation lock.

### Wi-Fi Security Auditing & Sanity Check Bypass
- **Weak Symbol Override:** Overrides Espressif's internal `ieee80211_raw_frame_sanity_check` to bypass libnet80211 raw frame restriction.
- **Deauth Frame Injection:** Target-specific or broadcast 802.11 deauthentication attacks.
- **Handshake Capture:** WPA/WPA2 4-Way Handshake sniffing & status monitor.
- **Beacon & Probe Flooding:** SSID beacon spamming and probe request flooding.
- **Packet Monitor & Channel Analyzer:** Real-time 802.11 frame type breakdown and RSSI channel tracking.
- **Rogue AP & Hidden SSID Reveal:** Rogue access point detection and hidden network name extraction.

### BLE & Bluetooth Classic Engine
- **BLE Scanner:** BLE advertisement scanning and device classification.
- **BLE Jammer:** Target-specific BLE device denial of service on BLE advertising channels (37, 38, 39 / 2402MHz, 2426MHz, 2480MHz).
- **Bluetooth Classic Sweep Jammer:** Full 79-channel spectrum sweep (2402–2480 MHz) at microsecond lock times with pseudorandom noise payload injection.

### nRF24 & ESP-01 Hardware Co-Processing
- **ESP-01 (ESP8266) UART Slave Integration:** Offloads 802.11 raw deauth frame injection and 2.4GHz BT interference tasks to an external ESP-01 module over UART, preserving ESP32-S3 CPU cycles and reducing thermal load.
- **Dedicated FSPI Bus:** FSPI bus initialization for nRF24L01+ transceiver control across 2.4GHz spectrum sweep jamming.

### Thermal Safety System
- Real-time ESP32-S3 internal chip temperature tracking using `esp_temp_sensor`.
- Automatic thermal throttling: RF modules automatically pause if internal CPU temperature exceeds 75°C, resuming when cooled down to 65°C.

### Smartwatch UI & Timekeeping
- **128x64 SSD1306 OLED** display with custom font rendering (`dram_font.h`).
- Real-time digital clock screen with battery level indicator.
- Automatic BLE time synchronization and manual RTC adjustment.

---

## Hardware Configuration & Pinout

### Component List
- **ESP32-S3 SuperMini Board** (Dual-core Xtensa LX7 @ 240 MHz, 4MB Flash, USB CDC On Boot)
- **SSD1306 128x64 OLED Display** (I2C)
- **ESP-01 (ESP8266) Module** (Optional UART Co-processor)
- **nRF24L01+ Radio Module** (FSPI / SPI2)
- **3.7V LiPo Battery & Charging Module**
- **Navigation Buttons** (Up, Down, Select / Enter)

### Pinout Connection Table

| Component | ESP32-S3 SuperMini GPIO | Description / Protocol |
|:---|:---|:---|
| **OLED SDA** | GPIO 8 | I2C Data Line |
| **OLED SCL** | GPIO 9 | I2C Clock Line (400 kHz) |
| **ESP-01 TX / RX** | UART (GPIO 20 / 21) | Serial Communication with ESP-01 |
| **nRF24 SCK** | GPIO 4 | Hardware FSPI Clock |
| **nRF24 MOSI** | GPIO 5 | Hardware FSPI Master Out Slave In |
| **nRF24 MISO** | GPIO 6 | Hardware FSPI Master In Slave Out |
| **nRF24 CSN** | GPIO 10 | SPI Chip Select |
| **nRF24 CE** | GPIO 7 | Chip Enable (Transmit Strobe) |
| **Button UP** | GPIO 2 | Input Pullup (Navigation Up) |
| **Button DOWN** | GPIO 3 | Input Pullup (Navigation Down) |
| **Button SELECT** | GPIO 10 / GPIO 1 | Input Pullup (Select / Mode Enter) |

---

## Required Libraries

Install via Arduino IDE Library Manager (`Ctrl+Shift+I`):
- `Adafruit SSD1306` by Adafruit
- `Adafruit GFX Library` by Adafruit
- `RF24` by TMRh20
- `ESP32 Board Package` (includes USB, USBHIDKeyboard, WiFi, BLEDevice, Preferences, esp_wifi)

---

## How to Flash

1. Open `deauther_watch.ino` in **Arduino IDE 2.x**.
2. Select Board Settings:
   - **Board:** `ESP32S3 Dev Module` or `ESP32-S3 SuperMini`
   - **USB CDC On Boot:** `Enabled`
   - **USB Mode:** `Hardware CDC and JTAG` (or `USB-OTG / TinyUSB` for BadUSB execution)
   - **Flash Size:** `4MB` (or `8MB`)
   - **CPU Frequency:** `240MHz`
3. Click **Upload** (`Ctrl+U`).

---

## Legal & Ethical Disclaimer

This project is created strictly for **educational research, scientific analysis, and authorized security auditing** in controlled laboratory environments where explicit consent has been granted. Always adhere to local telecommunication laws and regulations. The author assumes no liability for misuse.

---

## License

MIT License — See [LICENSE](LICENSE) for details.
