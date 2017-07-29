#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------
#define DEBUG_MODE false
#define BLUEFRUIT_SPI_CS 8
#define BLUEFRUIT_SPI_IRQ 7
#define BLUEFRUIT_SPI_RST 4
#define BLUEFRUIT_SPI_SCK 13
#define BLUEFRUIT_SPI_MISO 12
#define BLUEFRUIT_SPI_MOSI 11
// -----------------------------------------------------------------------------

// Dummy counter, just so we have something to send over bluetooth.
// Ultimately this represents a blood glucose level.
int couter = 0;

// Construct our bluetooth instance, using hardware SPI.
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/**
 * Error Message Helper.
 * 
 * Prints the error and blocks further execution.
 */
void error(const __FlashStringHelper *err)
{
    Serial.println(err);
    while (1);
}

/**
 * Primary Setup Method.
 */
void setup(void)
{
    // Wait for the Serial interface to be ready.
    // Required for Flora & Micro (probaby the Trinket to).
    while (!Serial); delay(500);

    // Output some output for anyone that might be listening.
    // A serial connection is not required to use this device.
    Serial.begin(115200);
    Serial.println(F("Libre FreeStyle NFC -> Bluetooth Relay Starting"));
    Serial.println(F("-----------------------------------------------"));

    // Initialise the Adafruit Bluetooth Module
    Serial.print(F("Initialising the Bluefruit LE module: "));
    if (!ble.begin(DEBUG_MODE)) error(F("Failed!"));
    if (!ble.factoryReset()) error(F("Factory Reset Failed!"));
    if (!ble.setMode(BLUEFRUIT_MODE_DATA)) error(F("DATA mode Failed!"));
    if (!ble.sendCommandCheckOK("AT+HWModeLED=BLEUART")) error(F("LED mode Failed!"));
    Serial.println(F("OK!"));

    // Output some extra details about the BLE Module when in debug mode.
    if (DEBUG_MODE)
    {
        Serial.println(F("Requesting Bluefruit info:"));
        ble.info();
    }

    // TODO: Setup the BM019 NFC Module
}

/**
 * Primary Loop Method.
 */
void loop(void)
{
    // TODO: Read blood glucose level from FreeStyle Libre via NFC.
    // Keeping mind the FreeStyle Libre device it's self does store the data over time.
    // So we should probably only read from NFC when someone is listenng via Bluetooth.
    couter++;

    if (ble.isConnected())
    {
        Serial.print("Sending count: ");
        Serial.println(couter);
        ble.println(couter);
    }
    else
    {
        Serial.print("Storing count: ");
        Serial.println(couter);
    }

    delay(500);
}
