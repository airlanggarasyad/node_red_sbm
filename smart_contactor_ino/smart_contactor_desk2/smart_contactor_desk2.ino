#include "DHT.h"
#include "PubSubClient.h"
#include <WiFiManager.h>
#include <AutoConnect.h>

#define BUTTON_PIN 32
#define RESET_BUTTON_PIN 22
#define RELAY_PIN 26

// Local MQTT
char *mqttServer = "192.168.0.106";
int mqttPort = 1883;

// Inisialisasi variabel
WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient mqttClient(wifiClient);

int led_state = 0;

unsigned long now = 0;
unsigned long now_reset = 0;
unsigned long previous_button_time = 0;
unsigned long previous_reset_button_time = 0;

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin yang akan digunakan
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);  
  pinMode(LED_BUILTIN, OUTPUT);  
  
  // Set initial condition untuk pilot lamp dan relay
  digitalWrite(RELAY_PIN, led_state);

  // Inisialisasi object WiFiManager ke variabel wifiManager
//  WiFiManager wifiManager;
//  wifiManager.resetSettings();

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(60);
  if (!wifiManager.autoConnect("Contactor #1")) {
    Serial.println("failed to connect and hit timeout");
  } else {
    Serial.println("Successfully connected");
    digitalWrite(LED_BUILTIN, HIGH);
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setelah terkoneksi langsung setup MQTT
  setupMQTT();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setupMQTT() {
  // Koneksi ke MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void reconnect() {
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()) {
      Serial.println("Reconnecting to MQTT Broker..");
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      
      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("Connected.");
        // Subscribe ke topic
        mqttClient.subscribe("smartContactor/desk2/lamp1");
        mqttClient.subscribe("smartContactor/desk2/status");
        mqttClient.subscribe("smartContactor/desk2/reset");
      }
      
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String receivedMsg;
  
  Serial.print("Messagge arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  
  for (int i = 0; i < length; i++) {
    receivedMsg += (char)payload[i];
  }
  
  Serial.print(receivedMsg);
  Serial.println();

  if (String(topic) == "smartContactor/desk2/lamp1") {
    if (receivedMsg == "ON") {
      digitalWrite(RELAY_PIN, LOW);
      mqttClient.publish("smartContactor/desk2/status", "ON");
    } else {
      digitalWrite(RELAY_PIN, HIGH);
      mqttClient.publish("smartContactor/desk2/status", "OFF");
    }
  }
  if (String(topic) == "smartContactor/desk2/reset") {
    if (receivedMsg == "RST") {
      resetWifi();
    }
  }
}

void resetWifi() {
  digitalWrite(LED_BUILTIN, LOW);
  wifiManager.resetSettings();
  ESP.restart();
}

void loop() {
  now = millis();
  now_reset = millis();

  if (now - previous_button_time > 250 && !digitalRead(BUTTON_PIN)) {
    led_state = !led_state;
    digitalWrite(RELAY_PIN, led_state);

    if (led_state) {
      mqttClient.publish("smartContactor/desk2/lamp1", "OFF");
      mqttClient.publish("smartContactor/desk2/status", "OFF");
    } else {
      mqttClient.publish("smartContactor/desk2/lamp1", "ON");
      mqttClient.publish("smartContactor/desk2/status", "ON");
    }
    
    previous_button_time = now;
  }

  if (now_reset - previous_reset_button_time > 250 && !digitalRead(RESET_BUTTON_PIN)) {
    Serial.println("reset");
    
    resetWifi();
    
    previous_reset_button_time = now_reset;
  }

  // Jika tidak terkoneksi dengan mqttClient maka coba koneksikan ulang
  if (!mqttClient.connected()) {
    reconnect();
  }
  
  mqttClient.loop();

}
