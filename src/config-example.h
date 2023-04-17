#define WIFI_SSID     "**********"
#define WIFI_PASSWORD "**********"

#define ID "room_comfort" // Add unique name for this sensor
#define INTERVAL 30 //seconds

#define DHTTYPE DHT11 // Type DHT 11

#define PIN_DHT 13    // Which pin is DHT 11 connected to
#define PIN_LED 2
#define PIN_BUTTON 5
#define PIN_LIGHT_SENSOR 32
// I2C
#define PIN_SDA 21
#define PIN_SCL 22

// For more information on where to get these values see: https://github.com/grafana/diy-iot/blob/main/README.md#sending-metrics
#define GC_PROM_URL "prometheus-us-central1.grafana.net"
#define GC_PROM_PATH "/api/prom/push"
#define GC_PORT 443
#define GC_PROM_USER "*****"
#define GC_PROM_PASS "********************"

#define TG_BOT_TOKEN "***************"
#define TG_CHAT_ID "**********"

#define CCS811_ADDR 0x5A
#define LCD_ADDR 0x27
#define TEMRATURE_LM75_ADDR 0x48

#define DEBUG_OFF_TIMER 60