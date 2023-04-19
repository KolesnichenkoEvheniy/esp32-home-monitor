#include "metrics.h"

// Prometheus client and transport
PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 4 series (when changing update buffers used to serialize the data)
WriteRequest req(7, 512 * 7);

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
TimeSeries ts1(5, "temperature_celsius", "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts2(5, "humidity_percent",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts3(5, "heat_index_celsius",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts4(5, "wifi_rssi",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts5(5, "eco2_ppm",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts6(5, "tvoc_ppb",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts7(5, "ambient_light_lux",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

// Function to set up Prometheus client
void setupPrometheusClient() {
  Serial.println("Setting up client...");
  
  uint8_t serialTimeout = 0;
  while (!Serial && serialTimeout < 50) {
    delay(100);
    Serial.println("Serial timeout: " + String(serialTimeout));
    serialTimeout++;
  }
  
  Serial.println("Configuring prometheus transport...");
  // Configure and start the transport layer
  transport.setUseTls(true);
  transport.setCerts(grafanaCert, strlen(grafanaCert));
  transport.setNtpServer((char*)"time.google.com");
  // transport.setWifiSsid(WIFI_SSID);
  // transport.setWifiPass(WIFI_PASSWORD);
  transport.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!transport.begin()) {
      Serial.println(transport.errmsg);
      while (true) {};
  }

  Serial.println("Configuring prometheus client...");
  // Configure the client
  client.setUrl(GC_PROM_URL);
  client.setPath((char*)GC_PROM_PATH);
  client.setPort(GC_PORT);
  client.setUser(GC_PROM_USER);
  client.setPass(GC_PROM_PASS);
  // client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!client.begin()) {
      Serial.println(client.errmsg);
      while (true) {};
  }

  // Add our TimeSeries to the WriteRequest
  req.addTimeSeries(ts1);
  req.addTimeSeries(ts2);
  req.addTimeSeries(ts3);
  req.addTimeSeries(ts4);
  req.addTimeSeries(ts5);
  req.addTimeSeries(ts6);
  req.addTimeSeries(ts7);
  // req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

  Serial.println("Prometheus transport configured successfully.");
}