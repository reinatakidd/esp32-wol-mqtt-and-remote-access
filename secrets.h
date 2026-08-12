// ============ SECRETS ============

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_USERNAME = "YOUR_MQTT_USERNAME";
const char* MQTT_PASSWORD = "YOUR_MQTT_PASSWORD";

// ============ VARIABLES ============

const char* MQTT_SERVER = "YOUR_MQTT_SERVER_ADDRESS";
const int MQTT_PORT = 8883;

// ============ PC CONFIGURATION ============

// Get this from `ipconfig /all`.
//
// Example:
// A6-3D-91-7B-42-E8
//
// Enter it as six hexadecimal bytes.
byte PC_MAC[] = {
  0xA6, 0x3D, 0x91, 0x7B, 0x42, 0xE8
};

// Find this from your local IP address and subnet mask.
//
// Example:
// 192.168.8.x + 255.255.255.0
// = 192.168.8.255
IPAddress BROADCAST_IP(192, 168, 8, 255);