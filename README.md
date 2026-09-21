# Deauther Watch v2.0 — ESP32-S3 SuperMini Smartwatch & BadUSB Multi-Tool

**Deauther Watch v2.0** is an advanced wearable wireless security research smartwatch, BadUSB HID payload injector, and dual-radio attack platform built on the **ESP32-S3 SuperMini** board paired with an **ESP-01 (ESP8266)** co-processor and **nRF24L01+** radio module.

---

## Architecture & Hardware Overview

The watch utilizes a multi-chip architecture to distribute processing load and thermal dissipation:

- **Primary MCU (ESP32-S3 SuperMini):** Manages the OLED UI, navigation buttons, RTC timekeeping, BLE/Bluetooth Classic operations, BadUSB HID execution, and overall state machine.
- **Secondary Co-Processor (ESP-01 / ESP8266):** Flashed with **Spacehuhn's `esp8266_deauther_2.6.1_NODEMCU.bin`** firmware. Communicates with ESP32-S3 over UART to offload 802.11 Deauth packet injection and 2.4GHz BT interference without locking the primary MCU.
- **RF Co-Processor (nRF24L01+):** Operates on custom FSPI (SPI2) for 2.4GHz spectrum sweep jamming, BLE advertising channel disruption, and BT Classic noise injection.

---

## Complete Feature Breakdown

### 1. BadUSB HID Engine (Turkish Q Keyboard Layout)
- **Native USB HID Stack:** Uses ESP32-S3 USB-OTG hardware CDC / TinyUSB stack.
- **Full Turkish Q Mapping:** Full mapping for special Turkish characters (`ğ`, `ü`, `ş`, `ı`, `ö`, `ç`, `İ`, `Ğ`, `Ü`, `Ş`, `Ö`, `Ç`).
- **Pre-loaded Payloads:**
  - `SysInfo`: Dumps OS version, hostname, user, architecture, and total RAM via PowerShell in a native GUI dialog box.
  - `AddAdmin`: Creates a hidden local administrator account (`HiddenOps`) and injects registry stealth keys.
  - `WiFiPass`: Extracts all stored Windows Wi-Fi SSIDs and cleartext passwords into a popup window.
  - `DefenderToggle`: Toggles Windows Defender real-time protection.
  - `AMSIBypass`: Memory patches AMSI (Antimalware Scan Interface) logging in PowerShell.
  - `LockScreen`: Instantly locks the Windows workstation.

### 2. Wi-Fi Security Auditing & Driver Bypass
- **Raw Frame Sanity Check Override:** Overrides Espressif's internal `ieee80211_raw_frame_sanity_check` function to allow raw 802.11 frame injection directly from ESP32-S3.
- **ESP-01 Dual-Radio Offloading:** Sends serial commands to the ESP-01 running `esp8266_deauther_2.6.1` for dedicated deauthentication bursts.
- **WPA/WPA2 Handshake Sniffer:** Captures 4-Way EAPOL Handshake packets and displays handshake status in real time.
- **Beacon Spam & Probe Flooding:** Broadcasts custom/random SSID beacon frames and floods probe request packets.
- **Rogue AP & Hidden Network Reveal:** Detects rogue APs and extracts hidden network SSIDs.
- **Packet Monitor & Channel Analyzer:** Displays live 802.11 frame types (Management, Control, Data, Deauth) and RSSI channel heatmaps.

### 3. BLE & Bluetooth Classic Engine
- **BLE Scanner:** Scans and classifies nearby BLE advertising devices.
- **BLE Advertising Jammer:** Targeted jamming on BLE advertising channels 37 (2402 MHz), 38 (2426 MHz), and 39 (2480 MHz).
- **Bluetooth Classic 79-Channel Sweep Jammer:** Sweeps all 79 BT Classic channels (2402–2480 MHz) with pseudorandom noise payloads to disrupt FHSS connections.

### 4. Thermal Safety & Power Management
- Internal CPU temperature reading using `esp_temp_sensor`.
- Automatic thermal throttling: RF modules pause if internal CPU temperature hits **75°C** and automatically resume when cooled down to **65°C**.

### 5. Smartwatch UI & Timekeeping
- Custom font rendering (`dram_font.h`) on 128x64 SSD1306 OLED.
- Digital clock face showing time, date, thermal status, and battery percentage.
- BLE automatic time sync and manual RTC adjustment.

---

## Hardware Pinout & Wiring Table

### Primary Pinout (ESP32-S3 SuperMini)

| Component | ESP32-S3 SuperMini Pin | Description / Protocol |
|:---|:---|:---|
| **OLED SDA** | GPIO 8 | I2C Data (400 kHz) |
| **OLED SCL** | GPIO 9 | I2C Clock (400 kHz) |
| **ESP-01 TX** | GPIO 20 (RX1) | Connects to ESP-01 RX |
| **ESP-01 RX** | GPIO 21 (TX1) | Connects to ESP-01 TX |
| **nRF24 SCK** | GPIO 4 | FSPI Clock |
| **nRF24 MOSI** | GPIO 5 | FSPI Master Out Slave In |
| **nRF24 MISO** | GPIO 6 | FSPI Master In Slave Out |
| **nRF24 CSN** | GPIO 10 | SPI Chip Select |
| **nRF24 CE** | GPIO 7 | Chip Enable (Transmit Strobe) |
| **Button UP** | GPIO 2 | Navigation Up (Input Pullup) |
| **Button DOWN** | GPIO 3 | Navigation Down (Input Pullup) |
| **Button SELECT** | GPIO 1 / GPIO 10 | Enter / Select (Input Pullup) |

### ESP-01 (ESP8266) Wiring Table

| ESP-01 Pin | Connection Point | Function |
|:---|:---|:---|
| **VCC** | 3.3V Regulator / Battery | Power Input (3.3V) |
| **GND** | Common GND | Ground |
| **TX** | ESP32-S3 GPIO 20 (RX1) | UART Serial Data to ESP32 |
| **RX** | ESP32-S3 GPIO 21 (TX1) | UART Serial Data from ESP32 |
| **CH_PD / EN**| 3.3V | Chip Enable (High) |
| **RST** | 3.3V (via 10k resistor) | Reset Line |
| **GPIO 0** | 3.3V (High for Normal Boot)| Low only during flashing |

---

## ESP-01 Flashing Instructions

The ESP-01 module must be flashed with Spacehuhn's Deauther v2.6.1 firmware prior to assembly:

1. Download binary: [`esp8266_deauther_2.6.1_NODEMCU.bin`](https://github.com/SpacehuhnTech/esp8266_deauther/releases/download/2.6.1/esp8266_deauther_2.6.1_NODEMCU.bin)
2. Connect ESP-01 to a USB-to-TTL Serial adapter (set GPIO 0 to GND for flash mode).
3. Use **esptool** or **NodeMCU PyFlasher**:
   ```bash
   esptool.py --port COMx --baud 115200 write_flash -fm dio 0x00000 esp8266_deauther_2.6.1_NODEMCU.bin
   ```
4. Disconnect GPIO 0 from GND and reboot the ESP-01.

---

## ESP32-S3 Flashing Instructions

1. Open `deauther_watch.ino` in **Arduino IDE 2.x**.
2. Install required libraries: `Adafruit SSD1306`, `Adafruit GFX`, `RF24`.
3. Board Settings:
   - **Board:** `ESP32S3 Dev Module` (or `ESP32-S3 SuperMini`)
   - **USB CDC On Boot:** `Enabled`
   - **USB Mode:** `Hardware CDC and JTAG` (or `TinyUSB / USB-OTG` for BadUSB)
   - **Flash Size:** `4MB` (or `8MB`)
   - **CPU Frequency:** `240MHz`
4. Click **Upload** (`Ctrl+U`).

---

## Legal & Ethical Disclaimer

This project is created strictly for **educational research, scientific analysis, and authorized security auditing** in controlled laboratory environments where explicit consent has been granted. Always adhere to local telecommunication laws and regulations. The author assumes no liability for misuse.

---

## License

MIT License — See [LICENSE](LICENSE) for details.
