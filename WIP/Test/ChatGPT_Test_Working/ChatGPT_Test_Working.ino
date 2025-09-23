// ESP32 + GC9A01 (round 240x240), microSD, RC522 RFID, Animated GIF player
// Plays /image.gif from SD on the round TFT and prints RFID UIDs to Serial.
//
// Board: ESP32 Dev Module
// PSRAM: Disabled (not required)

#include <SPI.h>
#include <SD.h>
#include <FS.h>                 // for File
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <AnimatedGIF.h>        // v2.x API (open returns void*)
#include <MFRC522.h>

// ---------- Pin assignments (your wiring) ----------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4    // set to -1 if your module auto-resets
#define TFT_BL   -1   // backlight not controlled (set to a pin if supported)

#define SD_CS    13

#define RFID_SS  21   // RC522 "SDA"/SS
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

// Centering offsets for the GIF on the 240x240 display
int16_t gifXOffset = 0, gifYOffset = 0;

// A small line buffer for pushing one scanline at a time to the TFT
static uint16_t lineBuf[240];

// ------------- AnimatedGIF file I/O callbacks (v2.x) --------------
// Return a handle we can carry around (a pointer to a dynamically
// allocated File). The library stores it in GIFFILE::fHandle.
void* GIFOpenFile(const char *fname, int32_t *pSize) {
  File *pf = new File;                 // allocate a File object on heap
  *pf = SD.open(fname, FILE_READ);
  if (!(*pf)) {                        // failed to open
    delete pf;
    return nullptr;
  }
  *pSize = pf->size();
  return (void*)pf;                    // this becomes GIFFILE::fHandle
}

void GIFCloseFile(void *pHandle) {
  File *pf = (File*)pHandle;
  if (pf) {
    pf->close();
    delete pf;
  }
}

// Must return number of bytes read (int32_t)
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *pf = (File*)pFile->fHandle;
  if (!pf) return 0;
  int32_t bytes = pf->read(pBuf, iLen);
  pFile->iPos = pf->position();
  return bytes;
}

// Return the new absolute position (int32_t)
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

  uint8_t  *s   = pDraw->pPixels;    // 8-bit indices
  uint16_t *pal = (uint16_t*)pDraw->pPalette; // RGB565 palette

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

// Helper: play /image.gif once (returns false if failed to open)
bool playGifOnce(const char *path) {
  if (!gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    Serial.println("Failed to open GIF.");
    return false;
  }

  // Center the GIF on the 240x240 round display
  gifXOffset = (tft.width()  - gif.getCanvasWidth())  / 2;
  gifYOffset = (tft.height() - gif.getCanvasHeight()) / 2;

  // Clear background
  tft.fillScreen(0x0000);

  // Play until done
  while (gif.playFrame(true, nullptr)) {
    yield();

    // Non-blocking RFID poll
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      Serial.print("RFID UID: ");
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
        Serial.print(rfid.uid.uidByte[i], HEX);
        Serial.print(i + 1 < rfid.uid.size ? ":" : "");
      }
      Serial.println();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  gif.close();
  return true;
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

  // Start TFT (safe 40MHz SPI)
  tft.begin(40000000);
  tft.setRotation(0);
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

  // Configure GIF palette endianness for RGB565
  // was:
  //gif.begin(GIF_PALETTE_RGB565_BE); // switch to *_LE if colors look swapped
  // try:
  gif.begin(GIF_PALETTE_RGB565_LE);


  Serial.println("Setup complete. Playing /image.gif ...");
}

void loop() {
  if (!playGifOnce("/image.gif")) {
    tft.fillScreen(0x0000);
    tft.setTextColor(0xFFFF); tft.setTextSize(2);
    tft.setCursor(20, 110);
    tft.print("Missing /image.gif");
    delay(200);
  }
}
