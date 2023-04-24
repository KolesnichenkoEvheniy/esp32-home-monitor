#ifndef __MEASUREMENTS_H__
#define __MEASUREMENTS_H__
#include <Arduino.h>
#include "globals.h"
#include "metrics.h"

typedef struct struct_measurements {
  float humidity;
  float temperature;
  float hic;
  unsigned int ambient_light_lux;
  float eco2;
  float tvoc;
} struct_measurements;

void PerformMeasurements( void * pvParameters );

struct_measurements getFreshMeasurements();

#endif // __MEASUREMENTS_H__