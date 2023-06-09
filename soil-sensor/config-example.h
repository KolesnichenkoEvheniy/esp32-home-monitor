#include <Arduino.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

//MAC Address of the receiver
uint8_t broadcastAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

#define WIFI_SSID "********"

// ESP32 pin GPIO that connects to AOUT pin of moisture sensor
#define SOIL_MOISURE_SENSOR_PIN 4

#define SOIL_WET 929
#define SOIL_DRY 2553

// seconds
#define TIME_TO_SLEEP 30 * 60