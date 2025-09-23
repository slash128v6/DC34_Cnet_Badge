#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>      // Install this library with the Arduino Library Manager
                           // Don't forget to configure the driver for the display!
#include <AnimatedGIF.h>   // Install this library with the Arduino Library Manager

#define SD_CS_PIN 5 // Chip Select Pin (CS) for SD card Reader

AnimatedGIF gif;
File gifFile;              // Global File object for the GIF file
TFT_eSPI tft = TFT_eSPI(); 

const char *filename = "/image.gif";   // Change to load other gif files in images/GIF

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Declare SPIClass objects for HSPI and VSPI
//SPIClass *vspi = NULL;
// const int VSPI_MISO_PIN = 19;
// const int VSPI_MOSI_PIN = 23;
// const int VSPI_SCK_PIN = 18;
// const int VSPI_SS_PIN = 5;

SPIClass *hspi = NULL;
// Define custom pins if not using default IOMUX pins
const int HSPI_MISO_PIN = 12;
const int HSPI_MOSI_PIN = 13;
const int HSPI_SCK_PIN = 14;
const int HSPI_SS_PIN = 22;

//MFRC522DriverPinSimple ss_pin(22); // Create pin driver. See typical pin layout above.

//SPIClass &spiClass = SPI; // Alternative SPI e.g. SPI2 or from library e.g. softwarespi.

//const SPISettings spiSettings = SPISettings(SPI_CLOCK_DIV4, MSBFIRST, SPI_MODE0); // May have to be set if hardware is not fully compatible to Arduino specifications.

//MFRC522DriverSPI driver{ss_pin, spiClass, spiSettings}; // Create SPI driver.

//MFRC522 mfrc522{driver}; // Create MFRC522 instance.


void setup()
{
  Serial.begin(115200);

  /*
  // Initialize VSPI
  vspi = new SPIClass(VSPI);
  // If using custom pins: vspi->begin(VSPI_SCK_PIN, VSPI_MISO_PIN, VSPI_MOSI_PIN, VSPI_SS_PIN);
  vspi->begin(); // Uses default VSPI IOMUX pins
  pinMode(VSPI_SS, OUTPUT); // Set default SS pin as output
  */
  
  // Initialize HSPI
  hspi = new SPIClass(HSPI);
  // If using custom pins:
  hspi->begin(HSPI_SCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN, HSPI_SS_PIN);
  //hspi->begin(); // Uses default HSPI IOMUX pins
  pinMode(HSPI_SS_PIN, OUTPUT); // Set default SS pin as output

  // Example: Set up SPI settings for each bus
  //SPISettings settings1(1000000, MSBFIRST, SPI_MODE0); // For VSPI device
  SPISettings settings2(2000000, MSBFIRST, SPI_MODE1); // For HSPI device

/*
  tft.begin();
  tft.setRotation(2); // Adjust Rotation of your screen (0-3)
  tft.fillScreen(TFT_BLACK);

  // Initialize SD card
  Serial.println("SD card initialization...");
  if (!SD.begin(SD_CS_PIN,tft.getSPIinstance()))
  {
    Serial.println("SD card initialization failed!");
    return;
  }

  // Initialize SPIFFS
  Serial.println("Initialize SPIFFS...");
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS initialization failed!");
  }

  // Reformmating the SPIFFS to have space if a large GIF is loaded
  // You could run out of SPIFFS storage space if you load more than one image or a large GIF
  // You can also increase the SPIFFS storage space by changing the partition of the ESP32
  //
  Serial.println("Formatting SPIFFS...");
  SPIFFS.format(); // This will erase all the files, change as needed, SPIFFs is non-volatile memory
  Serial.println("SPIFFS formatted successfully.");

  // Open GIF file from SD card
  Serial.println("Openning GIF file from SD card...");
  File sdFile = SD.open(filename);
  if (!sdFile)
  {
    Serial.println("Failed to open GIF file from SD card!");
    return;
  }

  // Create a file in SPIFFS to store the GIF
  File spiffsFile = SPIFFS.open(filename, FILE_WRITE, true);
  if (!spiffsFile)
  {
    Serial.println("Failed to copy GIF in SPIFFS!");
    return;
  }

  // Read the GIF from SD card and write to SPIFFS
  Serial.println("Copy GIF in SPIFFS...");
  byte buffer[512];
  while (sdFile.available())
  {
    int bytesRead = sdFile.read(buffer, sizeof(buffer));
    spiffsFile.write(buffer, bytesRead);
  }

  spiffsFile.close();
  sdFile.close();

  // Initialize the GIF
  Serial.println("Starting animation...");
  gif.begin(BIG_ENDIAN_PIXELS);

*/

  mfrc522.PCD_Init();   // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

}

void loop()
{
  /*
  if (gif.open(filename, fileOpen, fileClose, fileRead, fileSeek, GIFDraw))
  {
    tft.startWrite(); // The TFT chip slect is locked low
    while (gif.playFrame(true, NULL))
    {
    }
    gif.close();
    tft.endWrite(); // Release TFT chip select for the SD Card Reader
  }
  */

  	if ( !mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));

}

// Callbacks for file operations for the Animated GIF Lobrary
void *fileOpen(const char *filename, int32_t *pFileSize)
{
  gifFile = SPIFFS.open(filename, FILE_READ);
  *pFileSize = gifFile.size();
  if (!gifFile)
  {
    Serial.println("Failed to open GIF file from SPIFFS!");
  }
  return &gifFile;
}

void fileClose(void *pHandle)
{
  gifFile.close();
}

int32_t fileRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos;
  if (iBytesRead <= 0)
    return 0;

  gifFile.seek(pFile->iPos);
  int32_t bytesRead = gifFile.read(pBuf, iLen);
  pFile->iPos += iBytesRead;

  return bytesRead;
}

int32_t fileSeek(GIFFILE *pFile, int32_t iPosition)
{
  if (iPosition < 0)
    iPosition = 0;
  else if (iPosition >= pFile->iSize)
    iPosition = pFile->iSize - 1;
  pFile->iPos = iPosition;
  gifFile.seek(pFile->iPos);
  return iPosition;
}
