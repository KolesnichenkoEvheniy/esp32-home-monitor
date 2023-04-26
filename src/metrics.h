#ifndef __METRICS_H__
#define __METRICS_H__
#include <Arduino.h>
#include <PrometheusArduino.h>
#include "measurements.h"
#include "certificates.h"
#include "config.h"

// Prometheus client and transport
extern PromLokiTransport transport;
extern PromClient client;

// Create a write request for 4 series (when changing update buffers used to serialize the data)
extern WriteRequest req;

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
extern TimeSeries ts1;
extern TimeSeries ts2;
extern TimeSeries ts3;
extern TimeSeries ts4;
extern TimeSeries ts5;
extern TimeSeries ts6;
extern TimeSeries ts7;

void setupPrometheusClient();

void logMeasurements(ClimateMeasurements &measurements);

#endif // __METRICS_H__