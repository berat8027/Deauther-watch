# Deauther Watch v2.0 — ESP32-S3 SuperMini Smartwatch & BadUSB Multi-Tool

Deauther Watch v2.0 is an advanced wearable wireless research device and BadUSB HID payload injection tool built on the ESP32-S3 SuperMini board. It combines Wi-Fi security auditing, BLE/Bluetooth scanning and jamming, nRF24 2.4GHz spectrum testing, BadUSB execution with Turkish Q keyboard layout support, and a real-time digital smartwatch interface into a single compact device.

---

## Key Features

### BadUSB HID Engine (Turkish Q Keyboard Support)
- Native USB HID keyboard emulation with full Turkish Q character mapping.
- Automated payload execution: Wi-Fi password extraction, admin user creation, AMSI bypass, Defender toggle, system info dump, and screen lock.

### Wi-Fi Security Auditing
- Raw frame sanity check bypass via weak symbol override (`ieee80211_raw_frame_sanity_check`).
- Deauth frame injection, WPA/WPA2 4-way Handshake capture, packet monitor, and channel analyzer.
- Rogue AP detection, SSID spoofer, and hidden SSID reveal.

### BLE & Bluetooth Classic Engine
- BLE device scanning, advertisement analysis, and BLE jamming modes.
- Bluetooth Classic interference mode.

### nRF24 2.4GHz Integration
- Dedicated nRF24 2.4GHz Wi-Fi band sweep jamming and channel testing.

### Smartwatch UI & Timekeeping
- 128x64 SSD1306 OLED interface with digital clock screen and battery level indicator.
- Automatic BLE time sync and manual RTC adjustment.

---

## Hardware Configuration & Pinout

### Required Components
- ESP32-S3 SuperMini Board
- SSD1306 128x64 I2C OLED Display
- nRF24L01+ Radio Module (SPI)
- LiPo Battery & Charging Module (3.7V)
- Push Buttons (Navigation & Select)

### Pinout Table

| Component | ESP32-S3 SuperMini Pin | Function |
|:---|:---|:---|
| OLED SDA | GPIO 8 | I2C Data |
| OLED SCL | GPIO 9 | I2C Clock (400kHz) |
| nRF24 SCK | GPIO 4 | SPI Clock |
| nRF24 MOSI | GPIO 5 | SPI MOSI |
| nRF24 MISO | GPIO 6 | SPI MISO |
| nRF24 CSN | GPIO 7 | SPI Chip Select |
| nRF24 CE | GPIO 8 / GPIO 1 | Chip Enable |
| Button UP | GPIO 2 | Input Pullup |
| Button DOWN | GPIO 3 | Input Pullup |
| Button SELECT | GPIO 10 | Input Pullup |

---

## Required Libraries

Install via Arduino IDE Library Manager (`Ctrl+Shift+I`):
- `Adafruit SSD1306` by Adafruit
- `Adafruit GFX Library` by Adafruit
- `RF24` by TMRh20
- `ESP32 Board Package` (includes USB, USBHIDKeyboard, WiFi, BLEDevice, Preferences)

---

## How to Flash

1. Open `deauther_watch.ino` (or root project file) in **Arduino IDE 2.x**.
2. Select Board Settings:
   - **Board:** `ESP32S3 Dev Module` or `ESP32-S3 SuperMini`
   - **USB CDC On Boot:** `Enabled`
   - **USB Mode:** `Hardware CDC and JTAG` (or `USB-OTG / TinyUSB` for BadUSB)
   - **Flash Size:** `4MB` or `8MB`
   - **CPU Frequency:** `240MHz`
3. Click **Upload** (`Ctrl+U`).

---

## Legal & Ethical Disclaimer

This project is created strictly for educational research, authorized penetration testing, and legal security auditing in environments where explicit permission has been granted. The author assumes no liability for misuse.

---

## License

MIT License — See [LICENSE](LICENSE) for details.
