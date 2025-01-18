#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT11
#define FAN_PIN 19  // GPIO pin for fan control

const char* ssid = "DIGIFIBRA-Ak6u";
const char* password = "FfdHsYKFX6tA";
const char* mqtt_server = "192.168.1.136";
const int mqtt_port = 1883;
const char* mqtt_topic_temp = "temperature";
const char* mqtt_topic_hum = "humidity";
const char* mqtt_topic_fan = "fan/control";  // MQTT topic for fan control

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);  // Fan OFF initially

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Set MQTT callback function

  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();  

  delay(2000); 

  float h = dht.readHumidity() * 10;
  float t = dht.readTemperature() * 10;

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h, 1);
  Serial.print("%  Temperature: ");
  Serial.print(t, 1);
  Serial.println("°C");

  publishToMQTT(mqtt_topic_temp, t);
  publishToMQTT(mqtt_topic_hum, h);
}

// ==================== MQTT Callback Function ====================
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Convert to string
  String message = String((char*)payload);

  Serial.print("MQTT Message Received: ");
  Serial.println(message);

  if (strcmp(topic, mqtt_topic_fan) == 0) {
    if (message == "ON") {
      digitalWrite(FAN_PIN, HIGH);  // Turn fan ON
      Serial.println("Fan TURNED ON");
    } else if (message == "OFF") {
      digitalWrite(FAN_PIN, LOW);   // Turn fan OFF
      Serial.println("Fan TURNED OFF");
    }
  }
}

// ==================== Helper Functions ====================
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT... ");
    if (client.connect("ESP32_Client")) {
      Serial.println("Connected to MQTT broker!");
      client.subscribe(mqtt_topic_fan);  // Subscribe to fan control topic
    } else {
      Serial.print("Failed (Error Code: ");
      Serial.print(client.state());
      Serial.println(") Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void publishToMQTT(const char* topic, float value) {
  char msg[10];
  dtostrf(value, 6, 2, msg);
  client.publish(topic, msg);
}
