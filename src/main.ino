#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------
#define DEBUG_MODE false
#define NFC_SS 10
#define NFC_IRQ 9
#define BLUEFRUIT_SS 8
#define BLUEFRUIT_IRQ 7
#define BLUEFRUIT_RST 4
// -----------------------------------------------------------------------------

// Dummy counter, just so we have something to send over bluetooth.
// Ultimately this represents a blood glucose level.
int couter = 0;

byte NFC_TXBuffer[40]; // transmit buffer
byte NFC_RXBuffer[40]; // receive buffer
SPISettings nfcSPI(2000, MSBFIRST, SPI_MODE0);

// Construct our bluetooth instance, using hardware SPI.
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SS, BLUEFRUIT_IRQ, BLUEFRUIT_RST);

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

/* IDN_Command identifies the CR95HF connected to the Arduino.
This requires three steps.
1. send command
2. poll to see if CR95HF has data
3. read the response

If the correct response is received the serial monitor is used
to display the CR95HF ID number and CRC code.  This rountine is 
not that useful in using the NFC functions, but is a good way to 
verify connections to the CR95HF. 
*/
void IDN_Command()
{
    byte i = 0;

    // step 1 send the command
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0); // SPI control byte to send command to CR95HF
    SPI.transfer(1); // IDN command
    SPI.transfer(0); // length of data that follows is 0
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 2, poll for data ready
    // data is ready when a read byte
    // has bit 3 set (ex:  B'0000 1000')

    digitalWrite(NFC_SS, LOW);
    while (NFC_RXBuffer[0] != 8)
    {
        NFC_RXBuffer[0] = SPI.transfer(0x03); // Write 3 until
        NFC_RXBuffer[0] = NFC_RXBuffer[0] & 0x08; // bit 3 is set
    }
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 3, read the data
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0x02);            // SPI control byte for read
    NFC_RXBuffer[0] = SPI.transfer(0); // response code
    NFC_RXBuffer[1] = SPI.transfer(0); // length of data
    for (i = 0; i < NFC_RXBuffer[1]; i++)
        NFC_RXBuffer[i + 2] = SPI.transfer(0); // data
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    if ((NFC_RXBuffer[0] == 0) & (NFC_RXBuffer[1] == 15))
    {
        Serial.println("IDN COMMAND-"); //
        Serial.print("RESPONSE CODE: ");
        Serial.print(NFC_RXBuffer[0]);
        Serial.print(" LENGTH: ");
        Serial.println(NFC_RXBuffer[1]);
        Serial.print("DEVICE ID: ");
        for (i = 2; i < (NFC_RXBuffer[1]); i++)
        {
            Serial.print(NFC_RXBuffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
        Serial.print("ROM CRC: ");
        Serial.print(NFC_RXBuffer[NFC_RXBuffer[1]], HEX);
        Serial.print(NFC_RXBuffer[NFC_RXBuffer[1] + 1], HEX);
        Serial.println(" ");
    }
    else
    {
        error(F("BAD RESPONSE TO IDN COMMAND!"));
    }

    Serial.println(" ");
}

/* SetProtocol_Command programs the CR95HF for
ISO/IEC 15693 operation.

This requires three steps.
1. send command
2. poll to see if CR95HF has data
3. read the response

If the correct response is received the serial monitor is used
to display successful programming. 
*/
void SetProtocol_Command()
{
    byte i = 0;

    // step 1 send the command
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0x00); // SPI control byte to send command to CR95HF
    SPI.transfer(0x02); // Set protocol command
    SPI.transfer(0x02); // length of data to follow
    SPI.transfer(0x01); // code for ISO/IEC 15693
    SPI.transfer(0x0D); // Wait for SOF, 10% modulation, append CRC
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 2, poll for data ready

    digitalWrite(NFC_SS, LOW);
    while (NFC_RXBuffer[0] != 8)
    {
        NFC_RXBuffer[0] = SPI.transfer(0x03); // Write 3 until
        NFC_RXBuffer[0] = NFC_RXBuffer[0] & 0x08; // bit 3 is set
    }
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 3, read the data
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0x02);            // SPI control byte for read
    NFC_RXBuffer[0] = SPI.transfer(0); // response code
    NFC_RXBuffer[1] = SPI.transfer(0); // length of data
    digitalWrite(NFC_SS, HIGH);

    if (!((NFC_RXBuffer[0] == 0) & (NFC_RXBuffer[1] == 0)))
    {
        error(F("Failed to set BM019 for ISO/IEC 15693 Protocol!"));
    }
}

/* Inventory_Command chekcs to see if an RF
tag is in range of the BM019.

This requires three steps.
1. send command
2. poll to see if CR95HF has data
3. read the response

If the correct response is received the serial monitor is used
to display the the RF tag's universal ID.  
*/
void Inventory_Command()
{
    SPI.beginTransaction(nfcSPI);

    byte i = 0;

    // step 1 send the command
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0x00); // SPI control byte to send command to CR95HF
    SPI.transfer(0x04); // Send Receive CR95HF command
    SPI.transfer(0x03); // length of data that follows is 0
    SPI.transfer(0x26); // request Flags byte
    SPI.transfer(0x01); // Inventory Command for ISO/IEC 15693
    SPI.transfer(0x00); // mask length for inventory command
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 2, poll for data ready
    // data is ready when a read byte
    // has bit 3 set (ex:  B'0000 1000')

    digitalWrite(NFC_SS, LOW);
    while (NFC_RXBuffer[0] != 8)
    {
        NFC_RXBuffer[0] = SPI.transfer(0x03); // Write 3 until
        NFC_RXBuffer[0] = NFC_RXBuffer[0] & 0x08; // bit 3 is set
    }
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    // step 3, read the data
    digitalWrite(NFC_SS, LOW);
    SPI.transfer(0x02);            // SPI control byte for read
    NFC_RXBuffer[0] = SPI.transfer(0); // response code
    NFC_RXBuffer[1] = SPI.transfer(0); // length of data
    for (i = 0; i < NFC_RXBuffer[1]; i++)
        NFC_RXBuffer[i + 2] = SPI.transfer(0); // data
    digitalWrite(NFC_SS, HIGH);
    delay(1);

    SPI.endTransaction();
}

/**
 * Wake up the NFC Module.
 * 
 * The BM019 requires a wakeup pulse on its IRQ_IN pin before it will select
 * UART or SPI mode. The IRQ_IN pin is also the UART RX pin for DIN on the
 * BM019 board.
 */
void wakeUpBM019()
{
    digitalWrite(NFC_IRQ, HIGH);
    delay(10);
    digitalWrite(NFC_IRQ, LOW);
    delayMicroseconds(100);
    digitalWrite(NFC_IRQ, HIGH);
    delay(10);
}

/**
 * Primary Setup Method.
 */
void setup(void)
{
    // Wait for the Serial interface to be ready.
    // Required for Flora & Micro (probaby the Trinket to).
    while (!Serial); delay(500);

    // Output some info for anyone that might be listening.
    // A serial connection is not required to use this device.
    Serial.begin(9600);
    Serial.println(F("Libre FreeStyle NFC -> Bluetooth Relay Starting"));
    Serial.println(F("-----------------------------------------------"));

    // Initialise the Solutions Cubed NFC Module
    Serial.print(F("Initialising the NFC module: "));
    SPI.begin();
    SPI.beginTransaction(nfcSPI);
    pinMode(NFC_IRQ, OUTPUT);
    pinMode(NFC_SS, OUTPUT);
    digitalWrite(NFC_SS, HIGH);
    wakeUpBM019();
    if (DEBUG_MODE) IDN_Command();
    SetProtocol_Command();
    SPI.endTransaction();
    Serial.println(F("OK!"));

    // Initialise the Adafruit Bluetooth Module
    Serial.print(F("Initialising the BLE module: "));
    if (!ble.begin(DEBUG_MODE)) error(F("Failed!"));
    if (!ble.factoryReset()) error(F("Factory Reset Failed!"));
    if (!ble.setMode(BLUEFRUIT_MODE_DATA)) error(F("DATA mode Failed!"));
    if (!ble.sendCommandCheckOK("AT+HWModeLED=BLEUART")) error(F("LED mode Failed!"));
    if (DEBUG_MODE) ble.info();
    Serial.println(F("OK!"));
}


byte i = 0;

/**
 * Primary Loop Method.
 */
void loop(void)
{
    if (ble.isConnected())
    {
        Inventory_Command();

        if (NFC_RXBuffer[0] == 128)
        {
            Serial.print(F("TAG DETECTED - UID: "));
            ble.print(F("TAG DETECTED - UID: "));

            for (i = 11; i >= 4; i--)
            {
                Serial.print(NFC_RXBuffer[i], HEX);
                ble.print(NFC_RXBuffer[i], HEX);

                Serial.print(F(" "));
                ble.print(F(" "));
            }
        }
        else
        {
            Serial.print(F("NO TAG IN RANGE - "));
            ble.print(F("NO TAG IN RANGE - "));

            Serial.print(F("RESPONSE CODE: "));
            ble.print(F("RESPONSE CODE: "));

            Serial.print(NFC_RXBuffer[0], HEX);
            ble.print(NFC_RXBuffer[0], HEX);
        }

        Serial.println(F(" "));
        ble.println(F(" "));
    }
    else
    {
        Serial.println(F("Waiting for Bluetooth Connection..."));
    }

    delay(1000);
}
