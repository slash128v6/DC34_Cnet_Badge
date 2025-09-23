// ESP32 + GC9A01 (round 240x240), microSD, RC522 RFID, Animated GIF player with RFID switching
// Plays a GIF on the round TFT, switches GIF when a tag is presented.
//
// Board: ESP32 Dev Module
// PSRAM: Disabled

#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <AnimatedGIF.h>
#include <MFRC522.h>

// ---------- Pin assignments (your wiring) ----------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4     // set to -1 if your module auto-resets
#define TFT_BL   -1    // backlight not controlled (set to a pin if supported)

#define SD_CS    13

#define RFID_SS  21    // RC522 "SDA"/SS
#define RFID_RST 22

// VSPI pins (ESP32 DevKit): SCK=18, MISO=19, MOSI=23
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23
// -----------------------------------------------------------------

// Display
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// RFID
MFRC522 rfid(RFID_SS, RFID_RST);

// GIF decoder
AnimatedGIF gif;

// Playback state
static const char *DEFAULT_GIF = "/insert_slide.gif";
String currentGifPath = DEFAULT_GIF;
String lastUID = "";
unsigned long lastSwitchMs = 0;   // debounce

// Centering offsets for the GIF on the 240x240 display
int16_t gifXOffset = 0, gifYOffset = 0;

// A small line buffer for pushing one scanline at a time to the TFT
static uint16_t lineBuf[240];

// ---------- Tag → GIF mapping (edit these for your tags) ----------
struct TagMap { const char *uid; const char *path; };
// Use UIDs exactly as printed (uppercase hex, colon-separated).
TagMap tagMap[] = {
  // {"04:A1:B2:C3:D4:56:78", "/mifare1.gif"},
  // {"DE:AD:BE:EF",          "/deadbeef.gif"},
  //{"01:E4:1A:A2",          "/image.gif"},
  //{"2A:BD:A4:CD",          "/x_wing.gif"},
  {"01:E4:1A:A2",          "/Virus_2_240x240.gif"},
  {"2A:BD:A4:CD",          "/Virus_3_240x240.gif"},
  {"61:E9:F8:A3",          "/rick_roll_240x240.gif"},
  {"0A:9E:C4:CD",          "/matt_damon_240x240.gif"},
};
const size_t tagMapCount = sizeof(tagMap)/sizeof(tagMap[0]);
// ------------------------------------------------------------------

// ------------------ AnimatedGIF file I/O (v2.x) -------------------
void* GIFOpenFile(const char *fname, int32_t *pSize) {
  File *pf = new File;
  *pf = SD.open(fname, FILE_READ);
  if (!(*pf)) { delete pf; return nullptr; }
  *pSize = pf->size();
  return (void*)pf;
}

void GIFCloseFile(void *pHandle) {
  File *pf = (File*)pHandle;
  if (pf) { pf->close(); delete pf; }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *pf = (File*)pFile->fHandle;
  if (!pf) return 0;
  int32_t bytes = pf->read(pBuf, iLen);
  pFile->iPos = pf->position();
  return bytes;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *pf = (File*)pFile->fHandle;
  if (!pf) return -1;
  pf->seek(iPosition);
  pFile->iPos = iPosition;
  return iPosition;
}
// -------------------------------------------------------------------

// ------------- AnimatedGIF line-drawing callback -------------------
void GIFDraw(GIFDRAW *pDraw) {
  int16_t x = pDraw->iX + gifXOffset;
  int16_t y = pDraw->iY + pDraw->y + gifYOffset;
  int16_t w = pDraw->iWidth;
  if (w <= 0) return;

  // Clip
  if (x >= tft.width() || y < 0 || y >= tft.height()) return;
  if (x + w > tft.width()) w = tft.width() - x;

  uint8_t  *s   = pDraw->pPixels;           // 8-bit indices
  uint16_t *pal = (uint16_t*)pDraw->pPalette; // RGB565 (LE, we set in gif.begin)

  int16_t runStart = -1;
  for (int16_t i = 0; i < w; i++) {
    uint8_t idx = s[i];
    bool transparent = pDraw->ucHasTransparency && (idx == pDraw->ucTransparent);
    if (!transparent) {
      if (runStart < 0) runStart = i;
      lineBuf[i - runStart] = pal[idx];
    }
    bool endRun = (runStart >= 0) && (transparent || i == (w - 1));
    if (endRun) {
      int16_t runLen = i - runStart + (transparent ? 0 : 1);
      tft.startWrite();
      tft.setAddrWindow(x + runStart, y, runLen, 1);
      tft.writePixels(lineBuf, runLen, false);
      tft.endWrite();
      runStart = -1;
    }
  }
}
// -------------------------------------------------------------------

// ----------------- Helpers: UID → string / path --------------------
String uidToString(const MFRC522::Uid &uid) {
  String s; s.reserve(uid.size * 3);
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += '0';
    s += String(uid.uidByte[i], HEX);
    if (i + 1 < uid.size) s += ':';
  }
  s.toUpperCase();
  return s;
}

String removeColons(const String &u) {
  String out; out.reserve(u.length());
  for (size_t i = 0; i < u.length(); i++) if (u[i] != ':') out += u[i];
  return out;
}

String mappedPathForUID(const String &uidStr) {
  // 1) explicit map
  for (size_t i = 0; i < tagMapCount; i++) {
    if (uidStr.equals(tagMap[i].uid)) return String(tagMap[i].path);
  }
  // 2) fallback: "/<UID_NO_COLONS>.gif" if it exists
  String candidate = "/" + removeColons(uidStr) + ".gif";
  if (SD.exists(candidate)) return candidate;
  // 3) else keep current or default
  return String(DEFAULT_GIF);
}
// -------------------------------------------------------------------

/**
// Returns true if a *new* tag selected a *different* GIF (so caller can switch)
bool checkRFIDAndMaybeSwitch() {
  if (!(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())) return false;

  String uidStr = uidToString(rfid.uid);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // debounce: ignore the same UID spam for 1s unless it picks a new file
  unsigned long now = millis();
  bool allow = (uidStr != lastUID) || (now - lastSwitchMs > 1000);

  String newPath = mappedPathForUID(uidStr);
  bool isNewGif = (newPath != currentGifPath);

  if (allow && isNewGif) {
    Serial.print("RFID UID: "); Serial.println(uidStr);
    Serial.print("Switching to: "); Serial.println(newPath);
    currentGifPath = newPath;
    lastUID = uidStr;
    lastSwitchMs = now;
    return true;
  }
  return false;
}
*/

// Prints every scan. Only debounces the *switching* of GIFs.
bool checkRFIDAndMaybeSwitch() {
  if (!(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())) return false;

  String uidStr  = uidToString(rfid.uid);
  String newPath = mappedPathForUID(uidStr);

  // Always print each scan
  Serial.print("[SCAN] UID: "); Serial.println(uidStr);
  Serial.print("[SCAN] Candidate GIF: "); Serial.println(newPath);

  bool isNewGif = (newPath != currentGifPath);
  unsigned long now = millis();
  bool switchAllowed = isNewGif && (uidStr != lastUID || (now - lastSwitchMs > 1000));

  // tidy up the RF field session
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  if (switchAllowed) {
    Serial.println("[SCAN] Switching GIF.");
    currentGifPath = newPath;
    lastUID = uidStr;
    lastSwitchMs = now;
    return true;  // tell the player loop to break and reopen
  } else {
    if (!isNewGif) Serial.println("[SCAN] No switch (same GIF).");
    else           Serial.println("[SCAN] Switch suppressed by debounce.");
    // still record the last UID so debounce logic works, but printing always happens
    lastUID = uidStr;
    return false;
  }
}



// Play currentGifPath until it ends OR an RFID switch is requested.
// Returns true if it switched mid-playback.
bool playCurrentGifUntilSwitch() {
  if (!gif.open(currentGifPath.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    tft.fillScreen(0x0000);
    tft.setTextColor(0xFFFF); tft.setTextSize(2);
    tft.setCursor(10, 110);
    tft.print("Open fail:");
    tft.setCursor(10, 130);
    tft.print(currentGifPath);
    Serial.print("Failed to open "); Serial.println(currentGifPath);
    delay(1500);
    // fallback to default if current is bad
    if (currentGifPath != DEFAULT_GIF) currentGifPath = DEFAULT_GIF;
    return false;
  }

  // Center the GIF on the 240x240 round display
  gifXOffset = (tft.width()  - gif.getCanvasWidth())  / 2;
  gifYOffset = (tft.height() - gif.getCanvasHeight()) / 2;

  // Clear background
  tft.fillScreen(0x0000);

  bool switched = false;
  while (gif.playFrame(true, nullptr)) {
    yield();
    if (checkRFIDAndMaybeSwitch()) { switched = true; break; }
  }

  gif.close();
  return switched;
}

void setup() {
  // Keep all CS lines deasserted
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS,  OUTPUT); digitalWrite(SD_CS,  HIGH);
  pinMode(RFID_SS,OUTPUT); digitalWrite(RFID_SS,HIGH);

  if (TFT_BL >= 0) { pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH); }

  Serial.begin(115200);
  delay(200);

  // Shared SPI bus
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Start TFT
  tft.begin(40000000);         // try 80000000 if your module tolerates it
  tft.setRotation(2);
  tft.fillScreen(0x0000);

  // Start SD
  if (!SD.begin(SD_CS, SPI, 20000000)) {
    tft.setTextColor(0xFFFF); tft.setTextSize(2);
    tft.setCursor(10, 110);
    tft.print("SD init failed");
    Serial.println("SD init failed. Check wiring and SD_CS.");
    while (true) { delay(1000); }
  }

  // Start RC522
  rfid.PCD_Init(RFID_SS, RFID_RST);
  Serial.println("RC522 ready.");

  // GIF palette endianness (you found LE works best)
  gif.begin(GIF_PALETTE_RGB565_LE);
  // If available in your lib version, you can also:
  // gif.setDrawType(GIF_DRAW_COOKED);

  Serial.println("Setup complete. Tap a tag to switch GIFs.");
}

void loop() {
  // Play until end, then loop. If a tag switches mid-playback, it returns early.
  playCurrentGifUntilSwitch();
}
