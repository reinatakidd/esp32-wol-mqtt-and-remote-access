#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

#include "secrets.h"

// MQTT
const char* MQTT_TOPIC = "home/pc/wake";

// Certificate
const char* ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Networking
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
WiFiUDP udp;

// Timing / reliability
const unsigned long WIFI_TIMEOUT = 10000;
const unsigned long MQTT_RETRY_DELAY = 5000;

// Ignore additional WOL commands for 10 seconds
// after successfully processing one.
const unsigned long WOL_COOLDOWN = 10000;

unsigned long lastWakeTime = 0;

// Wi-Fi
bool connectWiFi() {

  Serial.print("Connecting to Wi-Fi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startAttempt < WIFI_TIMEOUT
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    WiFi.setSleep(false);

    Serial.println("Wi-Fi connected!");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    return true;
  } else {

    Serial.println("Wi-Fi connection failed.");

    return false;
  }
}

// Wake-on-LAN
void sendWakeOnLAN() {

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] >>> Sending Wake-on-LAN packet...");

  byte packet[102];

  for (int i = 0; i < 6; i++) {
    packet[i] = 0xFF;
  }

  for (int i = 0; i < 16; i++) {
    memcpy(
      &packet[6 + i * 6],
      PC_MAC,
      6
    );
  }

  udp.beginPacket(BROADCAST_IP, 9);
  udp.write(packet, sizeof(packet));
  udp.endPacket();

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] >>> Wake-on-LAN packet sent!");
}

// MQTT callback
void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] MQTT message received [");
  Serial.print(topic);
  Serial.print("]: ");

  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println(message);

  if (String(topic) != MQTT_TOPIC) {
    Serial.println(">>> Ignoring unexpected topic.");
    return;
  }

  if (message != "WAKE") {
    Serial.println(">>> Ignoring unknown command.");
    return;
  }

  unsigned long now = millis();

  if (
    lastWakeTime != 0 &&
    now - lastWakeTime < WOL_COOLDOWN
  ) {
    Serial.println(">>> WAKE ignored: cooldown active.");
    return;
  }

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] >>> WAKE command received!");

  sendWakeOnLAN();

  lastWakeTime = millis();
}

// MQTT connection
bool connectMQTT() {

  if (mqttClient.connected()) {
    return true;
  }

  Serial.print("\nConnecting to HiveMQ... ");

  String clientID =
    "ESP32-WOL-" +
    String((uint32_t)ESP.getEfuseMac(), HEX);

  if (
    mqttClient.connect(
      clientID.c_str(),
      MQTT_USERNAME,
      MQTT_PASSWORD
    )
  ) {

    Serial.println("connected!");

    if (mqttClient.subscribe(MQTT_TOPIC, 1)) {

      Serial.print("Subscribed to: ");
      Serial.println(MQTT_TOPIC);

      Serial.print("[");
      Serial.print(millis());
      Serial.println(" ms] MQTT subscription ready.");

      return true;
    } else {

      Serial.println(
        "ERROR: MQTT subscription failed!"
      );

      mqttClient.disconnect();

      return false;
    }

  } else {

    Serial.print("FAILED, MQTT state = ");
    Serial.println(mqttClient.state());

    return false;
  }
}

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 Wake-on-LAN MQTT");
  Serial.println("================================");

  WiFi.setAutoReconnect(true);

  connectWiFi();

  espClient.setCACert(ROOT_CA);

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );

  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(15);

  udp.begin(9);

  connectMQTT();

  Serial.println("\nESP32 ready.");
  }

void loop() {

  // Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED) {

    Serial.println();
    Serial.println("Wi-Fi connection lost.");

    connectWiFi();

    // If Wi-Fi still isn't available,
    // don't attempt MQTT yet.
    if (WiFi.status() != WL_CONNECTED) {

      delay(5000);

      return;
    }
  }

  // MQTT connection
  if (!mqttClient.connected()) {

    static unsigned long lastMQTTAttempt = 0;

    unsigned long now = millis();

    if (
      lastMQTTAttempt == 0 ||
      now - lastMQTTAttempt >= MQTT_RETRY_DELAY
    ) {

      lastMQTTAttempt = now;

      connectMQTT();
    }
  }

  mqttClient.loop();

  delay(10);
}