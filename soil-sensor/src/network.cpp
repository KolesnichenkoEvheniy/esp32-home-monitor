#include "network.h"

const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

String ssid;
String macAddress;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long previousMillis = 0;

// Search for parameter in HTTP POST request
const char* PARAM_SSID = "ssid";
const char* PARAM_MAC = "mac";
const char* PARAM_INTERVAL = "interval";

unsigned int refreshInterval = 30;

uint8_t* getServerMacAddress() {
  char *str = (char*)macAddress.c_str();
  uint8_t MAC[6];
  char* ptr;

  MAC[0] = strtol( strtok(str,":"), &ptr, HEX );
  for( uint8_t i = 1; i < 6; i++ ) {
    MAC[i] = strtol( strtok( NULL,":"), &ptr, HEX );
  }

  return MAC;
}

int32_t getWiFiChannel(const char *ssid) {
  if(ssid=="" || macAddress==""){
    Serial.println("Undefined SSID or mac.");
    return 0;
  }

  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

// // Initialize WiFi
// bool initWiFi() {
//   if(ssid=="" || macAddress==""){
//     Serial.println("Undefined SSID or mac.");
//     return false;
//   }

//   WiFi.mode(WIFI_STA);

//   //WiFi.begin(ssid.c_str(), pass.c_str());
//   // Serial.println("Connecting to WiFi...");

//   unsigned long currentMillis = millis();
//   previousMillis = currentMillis;

//   while(WiFi.status() != WL_CONNECTED) {
//     currentMillis = millis();
//     if (currentMillis - previousMillis >= interval) {
//       Serial.println("Failed to connect.");
//       return false;
//     }
//     delay(500);
//   }

//   return true;
// }

bool initEspNowClient() {
  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_AP_STA);


  Serial.println("WIFI Channel: " + String(channel));

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_wifi_start();
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  //Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

String processor(const String& var) {
  if(var == "INTERVAL") {
    return String(refreshInterval);
  } else if(var == "TEMPERATURE"){
    return String(measurements.temperature) + " C";
  } else if(var == "HUMIDITY"){
    return String(measurements.humidity) + " %";
  } else if(var == "CO2"){
    return String(measurements.eco2) + " ppm";
  } else if(var == "TVOC"){
    return String(measurements.tvoc) + "";
  }
  return String();
}

bool setupNetworkAndServer() {
    // Load values saved in SPIFFS
    ssid = readFile(SPIFFS, ssidPath);
    String macString = readFile(SPIFFS, passPath);
    refreshInterval = readFile(SPIFFS, intervalPath).toInt();
    if(refreshInterval < 1) refreshInterval = 5;

    int32_t channel = getWiFiChannel(macString.c_str());
    Serial.println(ssid);
    Serial.println(macString);
    Serial.println(refreshInterval);

    if(initWifiNowClient()) {
        
        delay(500);
        // Route for root / web page
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", "text/html", false, processor);
        });
        server.serveStatic("/", SPIFFS, "/");
    
        server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
            if (request->hasParam(PARAM_INTERVAL, true)) {
                refreshInterval = request->getParam(PARAM_INTERVAL, true)->value().toInt();
                Serial.println("New value: " + String(refreshInterval));
                writeFile(SPIFFS, intervalPath, request->getParam(PARAM_INTERVAL, true)->value().c_str());
                Serial.println("Refresh interval set to: " + String(refreshInterval));
            }
            request->send(200, "text/plain", "Saved. Refresh interval:" + String(refreshInterval));
        });

        server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
            Serial.println("Reset request");
        
            writeFile(SPIFFS, ssidPath, "");
            writeFile(SPIFFS, hostMacPath, "");
            writeFile(SPIFFS, intervalPath, "30");
            request->send(200, "text/plain", "Done. ESP will restart, connect to AP and go to IP address from the LCD");
            delay(3000);
            ESP.restart();
        });
        server.begin();
        return true;
    } else {
        // Connect to Wi-Fi network with SSID and password
        Serial.println("Setting AP (Access Point)");
        // NULL sets an open Access Point
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP); 
        // lcd.print("IP:" + IP.toString());

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
                if (p->name() == PARAM_SSID) {
                    ssid = p->value().c_str();
                    Serial.print("SSID set to: ");
                    Serial.println(ssid);
                    // Write file to save value
                    writeFile(SPIFFS, ssidPath, ssid.c_str());
                }
                // HTTP POST pass value
                if (p->name() == PARAM_MAC) {
                    pass = p->value().c_str();
                    Serial.print("Password set to: ");
                    Serial.println(pass);
                    // Write file to save value
                    writeFile(SPIFFS, passPath, pass.c_str());
                }
                //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                }
            }
            request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address.");
            delay(3000);
            ESP.restart();
        });
        server.begin();
        return false;
    }
}

