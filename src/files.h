#ifndef __FILES_H__
#define __FILES_H__
#include <Arduino.h>
#include "config.h"
#include "SPIFFS.h"

void initSPIFFS();

String readFile(fs::FS &fs, const char * path);

void writeFile(fs::FS &fs, const char * path, const char * message);

// File paths to save input values permanently
extern const char* ssidPath;
extern const char* passPath;
extern const char* intervalPath;

#endif // __FILES_H__