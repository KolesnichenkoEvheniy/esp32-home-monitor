#include <Arduino.h>
#include <Wire.h>
#include "DHTesp.h"
#include <Temperature_LM75_Derived.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
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

// Temperature sensor
Generic_LM75 temperature(TEMRATURE_LM75_ADDR);

LiquidCrystal_I2C lcd(LCD_ADDR,16,2);

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

// telegram setup

// UniversalTelegramBot bot(TG_BOT_TOKEN, secured_client);

int loopCounter = 0;
bool showDebugLight = false;
TimerHandle_t tmr;
int timerId = 1;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";


//Variables to save values from HTML form
String ssid;
String pass;
String ip;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
    delay(500);
  }

  Serial.println(WiFi.localIP());
  return true;
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

  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
  lcd.noCursor();

  Serial.println("The button is released, state: " + String(showDebugLight));
  blink((showDebugLight == true) ? 3 : 2);
}

void ping( TimerHandle_t xTimer )
{
  showDebugLight = false;
  Serial.println("TIMER!");

  lcd.setBacklight((showDebugLight == true) ? HIGH : LOW);
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

float averageAdcValue(int pin, int n = 5) {
  float sum = 0;

  for(int i=0; i < n; i++) {
    sum += analogRead(pin);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  return sum / n;
}

void PerformMeasurements( void * pvParameters ){
  Serial.println("Task2 running on core " + String(xPortGetCoreID()));

  for(;;){
    if(showDebugLight == true) {
      digitalWrite(PIN_LED, HIGH);
    }

    int64_t time;
    time = transport.getTimeMillis();

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
    float ambientLightReadingAdc = averageAdcValue(PIN_LIGHT_SENSOR);
    double ambientLightVoltage = ambientLightReadingAdc / 4095.0 * 3.3;
    float amps = ambientLightVoltage / 10000.0; // across 10,000 Ohms
    float microamps = amps * 1000000;
    int ambientLightLux = (int)(microamps * 2.0);

    Serial.println("TEMP6000 Sensor readings: ADC=" + String(ambientLightReadingAdc) + "Lux=" + String(ambientLightLux));

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(cels) || isnan(eCO2) || isnan(tvoc)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    Serial.println("Temperature: " + String(cels) +" Humidity: " + String(hum));
    Serial.println("CO2: "+String(eCO2)+"ppm, TVOC: "+String(tvoc)+"ppb");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("CO2:" + String((int)eCO2)+" TCO:"+String((int)tvoc));
    lcd.setCursor(0,1);
    lcd.print("T:" + String(cels) + "H:" + String((int)hum) + "L:" + String(ambientLightLux));

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
      ts7.resetSamples();
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
      if (!ts7.addSample(time, ambientLightLux)) {
        Serial.println(ts7.errmsg);
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

String processor(const String& var) {
  return "ON";
  // if(var == "STATE") {
  //   if(digitalRead(ledPin)) {
  //     ledState = "ON";
  //   }
  //   else {
  //     ledState = "OFF";
  //   }
  //   return ledState;
  // }
  // return String();
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

  lcd.init();
  lcd.backlight();

  // secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

  initSPIFFS();

  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);

  if(initWiFi()) {

    lcd.setCursor(0,0);
    lcd.print("IP:" + WiFi.localIP().toString());
    delay(500);
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // Route to set GPIO state to HIGH
    // server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    //   digitalWrite(ledPin, HIGH);
    //   request->send(SPIFFS, "/index.html", "text/html", false, String());
    // });

    // // Route to set GPIO state to LOW
    // server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    //   digitalWrite(ledPin, LOW);
    //   request->send(SPIFFS, "/index.html", "text/html", false, String());
    // });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Reset request");
  
      writeFile(SPIFFS, ssidPath, "");
      writeFile(SPIFFS, passPath, "");
      writeFile(SPIFFS, ipPath, "");
      request->send(200, "text/plain", "Done. ESP will restart, connect to AP and go to IP address from the LCD");
      delay(3000);
      ESP.restart();
    });
    server.begin();
  } else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 
    lcd.setCursor(0,0);
    lcd.print("IP:" + IP.toString());

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
    return;
  }

  setupClient();

  // bot.sendMessage(TG_CHAT_ID, "ESP started. WIFI connected. IP: " + WiFi.localIP().toString(), "");

  // Start the DHT sensor
  dht.setup(PIN_DHT, DHTesp::DHT11);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LIGHT_SENSOR, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  lcd.noBacklight();

  Serial.println("CCS811 Reading CO2 and VOC");
  Wire.begin(PIN_SDA, PIN_SCL);
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