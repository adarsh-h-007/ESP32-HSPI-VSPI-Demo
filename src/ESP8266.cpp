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

#include "printf.h"
#include <RF24.h>
#include <RF24Network.h>
#include<random>

RF24 radio(4, 5);  // nRF24L01(+) radio attached using Getting Started board

RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;   // Address of our node in Octal format ( 04,031, etc)
const uint16_t other_node = 01;  // Address of the other node in Octal format

/**** Create a large array for data to be received ****
* MAX_PAYLOAD_SIZE is defined in RF24Network_config.h
* Payload sizes of ~1-2 KBytes or more are practical when radio conditions are good
*/
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h

/* -------------------------- Function Prototyping -------------------------- */
void handleMessage(uint8_t *payload);
void respondWithRandomSpeedDirection(uint8_t *tagUID);
/* -------------------------------------------------------------------------- */

void setup(){
    Serial.begin(115200);
    while (!Serial) {
    // some boards need this because of native USB capability
    }
    Serial.println(F("\n/* -------------------------------------------------------------------------- */\n/*                                   ESP8266                                  */\n/* -------------------------------------------------------------------------- */"));

    if (!radio.begin()) {
        Serial.println(F("Radio hardware not responding!"));
        while (1) {
            // hold in infinite loop
    }
    }
    radio.setChannel(90);
    network.begin(/*node address*/ this_node);

    printf_begin();        // needed for RF24* libs' internal printf() calls
    //radio.printDetails();  // requires printf support
}

void loop(){
    network.update();   // Check the network regularly

    while (network.available())
    {
        RF24NetworkHeader header;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        network.read(header, &payload, sizeof(payload));
        handleMessage(payload);
    }
}

void handleMessage(uint8_t *payload){
    uint8_t msgType = payload[0];
    uint8_t tagUID[4];
    memcpy(tagUID, &payload[1], 4);

    //For Type 1 Queries
    if (msgType == 1){
        bool emergencySlowdown = payload[5];    //5th byte has the emergency slowdown status
        Serial.printf("Received Query Message\nTAG_UID: %02X%02X%02X%02X, Emergency Slowdown: %d\n",
                      tagUID[0], tagUID[1], tagUID[2], tagUID[3], emergencySlowdown);
        
        //Respond to the query with random speed ad direction
        respondWithRandomSpeedDirection(tagUID);
    }

    //For Type 3 Priority Updates
    if (msgType == 3){
        uint8_t priority = payload[6];
        uint8_t direction[2];
        memcpy(direction, &payload[8], 2);
        Serial.printf("Received Priority Update\nTAG_UID: %02X%02X%02X%02X, Priority: %d, Direction: %02X%02X\n\n",
                      tagUID[0], tagUID[1], tagUID[2], tagUID[3], priority, direction[0], direction[1]);
    }
}

void respondWithRandomSpeedDirection(uint8_t *tagUID){
    uint8_t message[10];    //Create a message array of 10 Bytes
    message[0] = 2; //Type 2 message
    memcpy(&message[1], tagUID, 4); //Copy Tag ID
    message[5] = 0xFF;  //Stuffing 5th Byte
    message[6] = 0xFF;  //Stuffing 6th Byte

    //Generate random speed
    uint8_t speed = random(50, 200);
    message[7] = speed; //Copy speed to 7th Byte

    //Generate random direction
    uint8_t dirPicker = random(0, 4);
    if (dirPicker == 0){
        message[8] = 0x1;
        message[9] = 0x0;
    }
    else if (dirPicker == 1){
        message[8] = 0x0;
        message[9] = 0x1;
    }
    else{
        message[8] = 0x1;
        message[9] = 0x1;
    }

    //Send the response(Type2)
    RF24NetworkHeader header(other_node);
    bool success = network.write(header, &message, sizeof(message));
    Serial.printf("Responded with TAG_UID: %02X%02X%02X%02X with Speed: %d cm/s, Direction: %02X%02X, Status: %s\n",
              tagUID[0], tagUID[1], tagUID[2], tagUID[3], speed, message[8], message[9], success ? "Success" : "Failed");
}