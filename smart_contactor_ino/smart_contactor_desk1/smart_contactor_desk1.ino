#include "DHT.h"
#include "PubSubClient.h"
#include <WiFiManager.h>
#include <AutoConnect.h>

#define BUTTON_PIN 12
#define LED_INDICATOR_PIN 26
#define RELAY_PIN 27

// Local MQTT
char *mqttServer = "broker.hivemq.com";
int mqttPort = 1883;

// Inisialisasi variabel
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int led_state = 0;

unsigned long now = 0;
unsigned long previous_button_time = 0;

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin yang akan digunakan
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);  
  pinMode(LED_BUILTIN, OUTPUT);  
  
  // Set initial condition untuk pilot lamp dan relay
  digitalWrite(LED_INDICATOR_PIN, led_state);
  digitalWrite(RELAY_PIN, led_state);

  // Inisialisasi object WiFiManager ke variabel wifiManager
  WiFiManager wifiManager;
  wifiManager.resetSettings();

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
        mqttClient.subscribe("smartContactor/desk1/lamp1");
        mqttClient.subscribe("smartContactor/desk1/status");
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

  if (String(topic) == "smartContactor/desk1/lamp1") {
    if(receivedMsg == "ON") {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(LED_INDICATOR_PIN, HIGH);
      
      mqttClient.publish("smartContactor/desk1/status", "ON");
    } else {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(LED_INDICATOR_PIN, LOW);
      
      mqttClient.publish("smartContactor/desk1/status", "OFF");
    }
  }
}

void loop() {
  now = millis();

  if (now - previous_button_time > 250 && !digitalRead(BUTTON_PIN)) {
    led_state = !led_state;
    digitalWrite(26, led_state);

    if (led_state) {
      mqttClient.publish("smartContactor/desk1/lamp1", "ON");
      mqttClient.publish("smartContactor/desk1/status", "ON");
    } else {
      mqttClient.publish("smartContactor/desk1/lamp1", "OFF");
      mqttClient.publish("smartContactor/desk1/status", "OFF");
    }
    
    previous_button_time = now;
  }

  // Jika tidak terkoneksi dengan mqttClient maka coba koneksikan ulang
  if (!mqttClient.connected()) {
    reconnect();
  }
  
  mqttClient.loop();

}
