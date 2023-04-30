#ifndef __NETWORK_H__
#define __NETWORK_H__
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <AsyncTCP.h>
#include "files.h"

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float soil;
  unsigned int readingId;
} struct_message;

bool setupNetworkAndServer();

// Create AsyncWebServer object on port 80
extern AsyncWebServer server;

#endif // __NETWORK_H__