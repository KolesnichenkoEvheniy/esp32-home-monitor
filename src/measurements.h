#ifndef __MEASUREMENTS_H__
#define __MEASUREMENTS_H__
#include <Arduino.h>
#include "globals.h"


typedef struct ClimateMeasurements {
  float humidity;
  float temperature;
  float hic;
  unsigned int ambient_light_lux;
  float eco2;
  float tvoc;
} ClimateMeasurements;

ClimateMeasurements getFreshMeasurements();

#endif // __MEASUREMENTS_H__