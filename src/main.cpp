#include <Arduino.h>
#include <Wire.h>
#include "DHTesp.h"
#include <WiFi.h>
// #include <WiFiClientSecure.h>
#include <PrometheusArduino.h>
#include <Adafruit_CCS811.h>
#include <LiquidCrystal_I2C.h>
// #include <UniversalTelegramBot.h>

#include "certificates.h"
#include "config.h"

// WiFiClientSecure secured_client;
// DHT Sensor
DHTesp dht;

// co2 sensoor
Adafruit_CCS811 ccs;

#ifdef LCD_ENABLED
LiquidCrystal_I2C lcd(0x27,16,2);
#endif // LCD_ENABLED

// Prometheus client and transport
PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 4 series (when changing update buffers used to serialize the data)
WriteRequest req(6, 512 * 6);

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
TimeSeries ts1(5, "temperature_celsius", "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts2(5, "humidity_percent",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts3(5, "heat_index_celsius",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

TimeSeries ts4(5, "wifi_rssi",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

TimeSeries ts5(5, "eco2_ppm",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");
TimeSeries ts6(5, "tvoc_ppb",  "{monitoring_type=\"room_comfort\",board_type=\"esp32_devkit1\",room=\"bedroom\"}");

// telegram setup

// UniversalTelegramBot bot(TG_BOT_TOKEN, secured_client);

int loopCounter = 0;
bool showDebugLight = false;
TimerHandle_t tmr;
int timerId = 1;

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

void toggleDebug( bool newDebugState ) {
  showDebugLight = newDebugState;
  #ifdef LCD_ENABLED
  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
  #endif // LCD_ENABLED

  Serial.println("The button is released, state: " + String(showDebugLight));
  blink((showDebugLight == true) ? 3 : 2);
}

void ping( TimerHandle_t xTimer )
{
  showDebugLight = false;
  Serial.println("TIMER!");

  #ifdef LCD_ENABLED
  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
  #endif // LCD_ENABLED
}

void ListenButton( void * pvParameters ){
  Serial.println("Task1 running on core " + String(xPortGetCoreID()));
  
  for(;;){
    if (digitalRead(PIN_BUTTON) == LOW) {
      vTaskDelay(20 / portTICK_PERIOD_MS);
      if (digitalRead(PIN_BUTTON) == LOW) {
        
        toggleDebug(!showDebugLight);
        if (showDebugLight)
        {
          Serial.println("Setting timer...");
          tmr = xTimerCreate("MyTimer", pdMS_TO_TICKS(DEBUG_OFF_TIMER * 1000), pdFALSE, ( void * )timerId, &ping);
          if( xTimerStart(tmr, 10 ) != pdPASS ) {
            Serial.println("Timer start error");
          }
        } 
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
    //Serial.println("DHT STATUS: "+String(dht.getStatus()));
    if (dht.getStatus() != 0) { //Judge if the correct value is read
      vTaskDelay(100 / portTICK_PERIOD_MS);
      goto flag; //If there is an error, go back to the flag and re-read the data
    }

    float tempratureCorrection = 1.4;

    float hum = newValues.humidity;
    float cels = newValues.temperature + tempratureCorrection;
    float eCO2, tvoc;

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(cels, hum, false);

    if(ccs.available()){
      //ccs.setEnvironmentalData(hum, cels);
      uint8_t ccsData = ccs.readData();
      Serial.println("CCS811 Sensor readings: " + String(ccsData));
      eCO2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(cels) || isnan(eCO2) || isnan(tvoc)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    Serial.println("Temperature: " + String(cels) +" Humidity: " + String(hum));
    Serial.println("CO2: "+String(eCO2)+"ppm, TVOC: "+String(tvoc)+"ppb");

    #ifdef LCD_ENABLED
    lcd.setCursor(0,0);
    lcd.print("CO2: " + String(eCO2)+"ppm.");
    lcd.setCursor(0,1);
    lcd.print("Temperature:" + String(cels));
    #endif // LCD_ENABLED

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
      ts5.resetSamples();
      ts6.resetSamples();
    } else {
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
      if (!ts5.addSample(time, eCO2)) {
        Serial.println(ts5.errmsg);
      }
      if (!ts6.addSample(time, tvoc)) {
        Serial.println(ts6.errmsg);
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

void scani2c() {
  byte error, address;
  int nDevices;
  Serial.println("Searching for I2C devices...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    } else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
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

  #ifdef LCD_ENABLED
  lcd.init();
  lcd.noBacklight();
  #endif // LCD_ENABLED

  Serial.println("CCS811 Reading CO2 and VOC");
  Wire.begin(SDA, SCL);
  if(!ccs.begin(CCS811_ADDR)){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  } else {
    Serial.println("CCS811 started");
  }
  // Wait for the sensor to be ready
  while(!ccs.available());

  scani2c();
  
  Serial.println("Creating tasks");
  xTaskCreate(ListenButton, "ListenButton", 10000, NULL, 1, NULL); 
  xTaskCreate(PerformMeasurements, "PerformMeasurements", 10000, NULL, 2, NULL);

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
 
  delay(500); 
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  
}