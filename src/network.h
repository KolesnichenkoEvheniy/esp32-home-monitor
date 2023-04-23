#ifndef __NETWORK_H__
#define __NETWORK_H__
#include <Arduino.h>
#include "globals.h"
#include "files.h"
#include "config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <AsyncTCP.h>

bool initWiFi();

bool setupNetworkAndServer();

void initESPNowClient();

extern String ssid;
extern String pass;

// Create AsyncWebServer object on port 80
extern AsyncWebServer server;

// Search for parameter in HTTP POST request
extern const char* PARAM_SSID;
extern const char* PARAM_PASSWORD;
extern const char* PARAM_INTERVAL;

#endif // __NETWORK_H__