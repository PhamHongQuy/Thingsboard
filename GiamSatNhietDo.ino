#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Định nghĩa các chân kết nối
#define DHTPIN 2  // Chân GPIO4 (D4)
#define DHTTYPE DHT11   // Loại cảm biến DHT11

#define RED_PIN 14    // Chân GPIO14 (D5)
#define GREEN_PIN 12  // Chân GPIO12 (D6)
#define BLUE_PIN 13   // Chân GPIO13 (D7)

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Ba Tam";
const char* password = "Ma8@0332815514";

const char* mqtt_server = "21b590631990418a8aafadd998ea2c2b.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "nodered";
const char* mqtt_password = "nodered123";

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

unsigned long timeDelay = millis();

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientID = "ESPClient-";
    clientID += String(random(0xffff), HEX);
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp8266/dht11");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage += (char)payload[i];
  Serial.println("Message arrived [" + String(topic) + "]" + incommingMessage);

  DynamicJsonDocument doc(100);
  DeserializationError error = deserializeJson(doc, incommingMessage);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
}

void publishMessage(const char* topic, String payload, boolean retained) {
  if (client.publish(topic, payload.c_str(), retained))
    Serial.println("Message published [" + String(topic) + "]: " + payload);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);

  dht.begin();

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);

  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void setColor(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

unsigned long timeUpdata = millis();
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - timeUpdata > 2000) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Kiểm tra nếu việc đọc thất bại
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Không thể đọc dữ liệu từ cảm biến DHT11");
      return;
    }

    // Thiết lập màu cho đèn LED RGB dựa trên nhiệt độ
    if (temperature > 30) {
      setColor(255, 0, 0); // Đỏ
    } else if (temperature >= 20 && temperature <= 30) {
      setColor(0, 255, 0); // Xanh lá
    } else {
      setColor(0, 0, 255); // Xanh dương
    }

    DynamicJsonDocument doc(1024);
    doc["humidity"] = humidity;
    doc["temperature"] = temperature;
    char mqtt_message[128];
    serializeJson(doc, mqtt_message);
    publishMessage("esp8266/dht11", mqtt_message, true);

    timeUpdata = millis();
  }
}
