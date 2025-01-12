/*
+=====+====================+===========================+
| Byte| Field Name         | Description               |
+=====+====================+===========================+
|  0  | Message Type       | 1 = Query                 |
|     |                    | 2 = Response to Query     |
|     |                    | 3 = Priority Assignment   |
+-----+--------------------+---------------------------+
| 1-4 | RFID Unique ID     | Unique ID of RFID Tag     |
+-----+--------------------+---------------------------+
|  5  | Emergency Slowdown | Used in Type 1; stuffed   |
|     |                    | with 0xFF in others       |
+-----+--------------------+---------------------------+
|  6  | Priority Level     | Used in Type 3; stuffed   |
|     |                    | with 0xFF in others       |
+-----+--------------------+---------------------------+
|  7  | Vehicle Speed      | Relevant in Type 2 & 3;   |
|     |                    | stuffed in Type 1         |
+-----+--------------------+---------------------------+
| 8-9 | Vehicle Direction  | 0x01 0x00 = Left          |
|     |                    | 0x00 0x01 = Right         |
|     |                    | 0x01 0x01 = Straight      |
+=====+====================+===========================+

*/

#include <SPI.h>
#include <MFRC522.h>
#include "printf.h"
#include <RF24.h>
#include <RF24Network.h>

/* --------------------------- RFID Initialization -------------------------- */
extern SPIClass HSPIRFID ;  //To specify that HSPIRFID has been defined in another file

#define RST_PIN 27  // Configurable, see typical pin layout above
#define SS_PIN  26   // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
/* -------------------------------------------------------------------------- */

/* ------------------------- NRF24L01 Initialization ------------------------ */
RF24 radio(22, 21);  // nRF24L01(+) radio attached using Getting Started board

RF24Network network(radio);  // Network uses that radio

const uint16_t this_node = 01;   // Address of our node in Octal format
const uint16_t other_node = 00;  // Address of the other node in Octal format

/**** Create a large array for data to be sent ****
* MAX_query_SIZE is defined in RF24Network_config.h
* query sizes of ~1-2 KBytes or more are practical when radio conditions are good
*/
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];
/* -------------------------------------------------------------------------- */

/* --------------------------- Fuction Prototyping -------------------------- */
void SendQuery(uint8_t *tagUID, bool EmergencySlowdown);
void HandleResponse(uint8_t *payload);
uint8_t computePriority(uint16_t speed, uint8_t *direction);
void SendPriorityUpdate(uint8_t *tagUID, uint8_t priority, uint8_t speed, uint8_t *direction);
/* -------------------------------------------------------------------------- */

void setup() {
    /* ----------------------------- NRF24L01 Setup ----------------------------- */
    Serial.begin(115200);
    while (!Serial) {
        // some boards need this because of native USB capability
    }

    Serial.println(F("/* -------------------------------------------------------------------------- */\n/*                                    ESP32                                   */\n/* -------------------------------------------------------------------------- */"));
    
    printf_begin();  // needed for RF24* libs' internal printf() calls


    if (!radio.begin()) {
        Serial.println(F("Radio hardware not responding!"));
        while (1) {
        // hold in infinite loop
        }
    }
    radio.setChannel(90);
    network.begin(/*node address*/ this_node);
    //radio.printDetails();

    uint16_t sizeofSend = 0;  //Variable to indicate how much data to send
    bool stopSending = 0;     //Used to stop/start sending of data
    /* -------------------------------------------------------------------------- */

    /* ------------------------------- RFID Setup ------------------------------- */
    HSPIRFID.begin();           // Initialize HSPI bus
    mfrc522.PCD_Init();    // Initialize the MFRC522 reader
    delay(50);             // Allow the reader to stabilize
    Serial.println(F("Place an RFID tag to scan its UID..."));
    /* -------------------------------------------------------------------------- */
}

void loop(){
    //Awaiting response
    network.update();
    while (network.available()){
        RF24NetworkHeader header;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        network.read(header, &payload, sizeof(payload));
        HandleResponse(payload);
    }

        // Check if a new card is present
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    // Try to read the card's serial number
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    // Retrieve the tag UID
    uint8_t tagUID[4];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        tagUID[i] = mfrc522.uid.uidByte[i];
    }

    //Print the ID of scanned tag
    Serial.printf("Tag detected. UID: %02X%02X%02X%02X\n" , tagUID[0], tagUID[1], tagUID[2], tagUID[3]);

    //Send query
    SendQuery(tagUID, false);

    //Awaiting response
    network.update();
    while (network.available()){
        RF24NetworkHeader header;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        network.read(header, &payload, sizeof(payload));
        HandleResponse(payload);
    }
    // Halt the card to prepare for the next read
    mfrc522.PICC_HaltA();
}


void SendQuery(uint8_t *tagUID , bool EmergencySlowdown){
    uint8_t query[10];    //Initialize query array of 10 Bytes size
    query[0] = 1; //Query, so Type 1
    memcpy(&query[1], tagUID, 4); //Copy the tag's UID from Byte 1 to 4
    query[5] = EmergencySlowdown ? 1 : 0;
    //Filling the rest of the query with 0xFF
    for (int i = 6; i < sizeof(query); i++){
        query[i] = 0xFF;
    }

    //Sending the query
    RF24NetworkHeader header(other_node);
    bool success = network.write(header, &query, sizeof(query));
    Serial.println(success ? F("Query sent successfully.") : F("Query failed."));
}

void HandleResponse(uint8_t *payload){
    uint8_t msgType = payload[0];
    uint8_t tagUID[4];
    memcpy(tagUID, &payload[1], 4); //Copy Tag ID 

    if (msgType == 2){
        uint8_t speed = payload[7];
        uint8_t direction[2];
        memcpy(direction, &payload[8], 2);
        Serial.printf("Response Received\nTAG UID: %02X%02X%02X%02X, Speed: %d cm/s, Direction: %02X%02X\n",
                      tagUID[0], tagUID[1], tagUID[2], tagUID[3], speed, direction[0], direction[1]);
        // Compute priority and send update
        uint8_t priority = computePriority(speed, direction);
        network.update();
        SendPriorityUpdate(tagUID, priority, speed, direction);
    }
}

uint8_t computePriority(uint16_t speed, uint8_t *direction) {
    // Placeholder logic for computing priority
    // Emergency vehicles always get highest priority
    if (speed > 100) {
        return 255;
    }
    return 128;  // Normal priority for now
}

void SendPriorityUpdate(uint8_t *tagUID, uint8_t priority, uint8_t speed, uint8_t *direction){
    network.update();
    uint8_t UpdateMessage[10];
    UpdateMessage[0] = 3; // Type 3 Message
    memcpy(&UpdateMessage[1], tagUID, 4);   //Copy Tag ID
    UpdateMessage[5] = 0xFF;    //Bit stuffing
    UpdateMessage[6] = priority;
    UpdateMessage[7] = speed;
    memcpy(&UpdateMessage[8], direction, 2);

    network.update();
    RF24NetworkHeader header(other_node);
    bool success = network.write(header, &UpdateMessage, sizeof(UpdateMessage));
    Serial.println(success ? F("Priority update sent successfully.\n\n") : F("Priority update failed.\n\n"));
}