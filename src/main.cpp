#include <Arduino.h>
#include "DHTesp.h"
#include <WiFi.h>
// #include <WiFiClientSecure.h>
#include <PrometheusArduino.h>
// #include <UniversalTelegramBot.h>

#include "certificates.h"
#include "config.h"

// WiFiClientSecure secured_client;
// DHT Sensor
DHTesp dht;

// Prometheus client and transport
PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 4 series (when changing update buffers used to serialize the data)
WriteRequest req(4, 2048);

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
TimeSeries ts1(5, "temperature_celsius", "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts2(5, "humidity_percent",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts3(5, "heat_index_celsius",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

TimeSeries ts4(5, "wifi_rssi",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

// telegram setup

// UniversalTelegramBot bot(TG_BOT_TOKEN, secured_client);

int loopCounter = 0;
bool showDebugLight = true;

void setupWiFi() {
  Serial.println("Connecting to wifi ...'");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to set up Prometheus client
void setupClient() {
  Serial.println("Setting up client...");
  
  uint8_t serialTimeout = 0;
  while (!Serial && serialTimeout < 50) {
    delay(100);
    Serial.println("Serial timeout: " + String(serialTimeout));
    serialTimeout++;
  }

  // Setup wifi
  WiFi.mode(WIFI_STA);
  //wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.println("Configuring prometheus transport...");
  // Configure and start the transport layer
  transport.setUseTls(true);
  transport.setCerts(grafanaCert, strlen(grafanaCert));
  // transport.setNtpServer("time.google.com");
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
  // req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

  Serial.println("Prometheus transport configured successfully.");
}

void blink(int count = 2) {
  for(int i = 0; i < count; i++) {
    digitalWrite(PIN_LED, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(PIN_LED, LOW);
  }
}

void ListenButton( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){
    if (digitalRead(PIN_BUTTON) == LOW) {
      vTaskDelay(20 / portTICK_PERIOD_MS);
      if (digitalRead(PIN_BUTTON) == LOW) {
        showDebugLight = !showDebugLight;
        Serial.println("The button is released, state: " + String(showDebugLight));
        blink((showDebugLight == true) ? 3 : 2);
      }
      while (digitalRead(PIN_BUTTON) == LOW);
      vTaskDelay(20 / portTICK_PERIOD_MS);
      while (digitalRead(PIN_BUTTON) == LOW);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void PerformMeasurements( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    if(showDebugLight == true) {
      digitalWrite(PIN_LED, HIGH);
    }

    int64_t time;
    time = transport.getTimeMillis();

    // Read temperature and humidity
    flag:TempAndHumidity newValues = dht.getTempAndHumidity(); //Get the Temperature and humidity
    Serial.println("DHT STATUS: "+String(dht.getStatus()));
    if (dht.getStatus() != 0) { //Judge if the correct value is read
      vTaskDelay(100 / portTICK_PERIOD_MS);
      goto flag; //If there is an error, go back to the flag and re-read the data
    }

    float tempratureCorrection = 1.4;

    float hum = newValues.humidity;
    float cels = newValues.temperature + tempratureCorrection;
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(cels)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(cels, hum, false);

    if (loopCounter >= 5) {
      Serial.println("Sending samples...");
      //Send
      loopCounter = 0;
      PromClient::SendResult res = client.send(req);
      if (!res == PromClient::SendResult::SUCCESS) {
          Serial.println(client.errmsg);
      }
      Serial.println("Samples sent.");
      
      // Reset batches after a succesful send.
      ts1.resetSamples();
      ts2.resetSamples();
      ts3.resetSamples();
      ts4.resetSamples();
    } else {
      Serial.println("Temperature: " + String(cels) +" Humidity: " + String(hum));

      if (!ts1.addSample(time, cels)) {
        Serial.println(ts1.errmsg);
      }
      if (!ts2.addSample(time, hum)) {
        Serial.println(ts2.errmsg);
      }
      if (!ts3.addSample(time, hic)) {
        Serial.println(ts3.errmsg);
      }
      if (!ts4.addSample(time, WiFi.RSSI())) {
        Serial.println(ts4.errmsg);
      }
      loopCounter++;
    }

    ///////// ---
    Serial.println("Show light? " + String(showDebugLight));

    if(showDebugLight == true) {
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    digitalWrite(PIN_LED, LOW);
    // wait INTERVAL seconds and then do it again
    vTaskDelay(INTERVAL * 1000 / portTICK_PERIOD_MS);
  }
}

// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {
  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  // secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

  // Set up client
  setupWiFi();
  setupClient();

  // bot.sendMessage(TG_CHAT_ID, "ESP started. WIFI connected. IP: " + WiFi.localIP().toString(), "");

  // Start the DHT sensor
  dht.setup(PIN_DHT, DHTesp::DHT11);

  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  xTaskCreate(ListenButton, "ListenButton", 10000, NULL, 1, NULL); 
  xTaskCreate(PerformMeasurements, "PerformMeasurements", 10000, NULL, 2, NULL);

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
 
  delay(500); 
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  
}