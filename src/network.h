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

// Create AsyncWebServer object on port 80
extern AsyncWebServer server;

#endif // __NETWORK_H__