#ifndef __NETWORK_H__
#define __NETWORK_H__
#include <Arduino.h>
#include "globals.h"
#include "config.h"
#include "files.h"
#include "metrics.h"
#include "measurements.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <AsyncTCP.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float soil;
  unsigned int readingId;
} struct_message;

bool initWiFi();

bool setupNetworkAndServer();

void initESPNowClient();

void keepWiFiAlive(void * parameter);

// Create AsyncWebServer object on port 80
extern AsyncWebServer server;

#endif // __NETWORK_H__