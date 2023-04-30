#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "config.h"

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float soil;
  unsigned int readingId;
} struct_message;

//Create a struct_message called myData
struct_message myData;

unsigned long previousMillis = 0;   // Stores last time temperature was published

unsigned int readingId = 0;

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

float sensorReadings() {
  int soilMoistureValue = analogRead(SOIL_MOISURE_SENSOR_PIN);
  Serial.println("Sensor reasing: " + String(soilMoistureValue));
  return map(soilMoistureValue, SOIL_DRY, SOIL_WET, 0, 100);
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}
 
void setup() {
  //Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  pinMode(SOIL_MOISURE_SENSOR_PIN, INPUT);

  // parseBytes(RECIEVER_MAC_ADDRESS, '-', broadcastAddress, 6, HEX);
  // broadcastAddress = getServerMacAddress(RECIEVER_MAC_ADDRESS);


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
 
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= REFRESH_INTERVAL * 1000) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;

    float currentReadings = sensorReadings();
    Serial.println("Current moisture %: " + String(currentReadings));

    //Set values to send
    myData.id = BOARD_ID;
    myData.soil = currentReadings;
    myData.readingId = readingId++;
     
    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}