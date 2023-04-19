#include "network.h"

const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

String ssid;
String pass;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_SSID = "ssid";
const char* PARAM_PASSWORD = "pass";
const char* PARAM_INTERVAL = "interval";


// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || pass==""){
    Serial.println("Undefined SSID or Password.");
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

String processor(const String& var) {
  if(var == "INTERVAL") {
    return String(refreshInterval);
  }
  return String();
}

void setupNetworkAndServer() {
    lcd.backlight();
    initSPIFFS();

    // Load values saved in SPIFFS
    ssid = readFile(SPIFFS, ssidPath);
    pass = readFile(SPIFFS, passPath);
    refreshInterval = readFile(SPIFFS, intervalPath).toInt();
    if(refreshInterval < 1) refreshInterval = 5;
    Serial.println(ssid);
    Serial.println(pass);
    Serial.println(refreshInterval);

    if(initWiFi()) {
        lcd.setCursor(0,0);
        lcd.print("IP:" + WiFi.localIP().toString());
        lcd.setCursor(0,1);
        lcd.print("Refresh: " + String(refreshInterval));
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
            writeFile(SPIFFS, passPath, "");
            writeFile(SPIFFS, intervalPath, "30");
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
                if (p->name() == PARAM_SSID) {
                    ssid = p->value().c_str();
                    Serial.print("SSID set to: ");
                    Serial.println(ssid);
                    // Write file to save value
                    writeFile(SPIFFS, ssidPath, ssid.c_str());
                }
                // HTTP POST pass value
                if (p->name() == PARAM_PASSWORD) {
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
        return;
    }
}