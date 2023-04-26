#include "measurements.h"

ClimateMeasurements measurements;

void performFreshMeasurements() {
    // Read temperature and humidity
    flag:TempAndHumidity newValues = dht.getTempAndHumidity(); //Get the Temperature and humidity
    if (dht.getStatus() != 0) { //Judge if the correct value is read
      vTaskDelay(100 / portTICK_PERIOD_MS);
      goto flag; //If there is an error, go back to the flag and re-read the data
    }

    float hum = newValues.humidity;
    float cels = temperature.readTemperatureC();
    float eCO2, tvoc;

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(cels, hum, false);

    if(ccs.available()){
      ccs.setEnvironmentalData(hum, cels);
      uint8_t ccsData = ccs.readData();
      Serial.println("CCS811 Sensor readings: " + String(ccsData));
      eCO2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }

    // light
    float ambientLightReadingAdc = averageAdcValue(PIN_LIGHT_SENSOR, 5);
    double ambientLightVoltage = ambientLightReadingAdc / 4095.0 * 3.3;
    float amps = ambientLightVoltage / 10000.0; // across 10,000 Ohms
    float microamps = amps * 1000000;
    int ambientLightLux = (int)(microamps * 2.0);

    measurements.humidity = hum;
    measurements.temperature = cels;
    measurements.hic = hic;
    measurements.eco2 = eCO2;
    measurements.tvoc = tvoc;
    measurements.ambient_light_lux = ambientLightLux;
}