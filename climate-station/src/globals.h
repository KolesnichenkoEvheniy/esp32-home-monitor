#ifndef __GLOBALS_H__
#define __GLOBALS_H__
#include "config.h"
#include <Arduino.h>
#include <Adafruit_CCS811.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"
#include <Temperature_LM75_Derived.h>

// DHT Sensor
extern DHTesp dht;

// co2 sensoor
extern  Adafruit_CCS811 ccs;

// Temperature sensor
extern Generic_LM75 temperature;

extern LiquidCrystal_I2C lcd;

extern int refreshInterval;

// Timer variables
extern unsigned long previousMillis;

extern bool showDebugLight;

float averageAdcValue(int pin, int n);

void blink(int count);
void toggleDebug(bool newDebugState);

void scani2c();

#endif // __GLOBALS_H__