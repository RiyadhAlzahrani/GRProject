#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "HS"
#define WIFI_PASSWORD "12345678"

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 43, 101)
#define MQTT_PORT 1883

// MQTT Topics
#define MQTT_PUB_TEMP "Box1/Temp"
#define MQTT_PUB_HUM "Box1/Hum"
#define MQTT_PUB_SS "Box1/SS"

// Digital pin connected to the sensors
#define LedPin 5 //D1 
#define ShockPin 4 //D2
#define BottonPin 0 //D3
#define DHTPIN 14 //D5

// Uncomment whatever DHT sensor type you're using
#define DHTTYPE DHT11   // DHT 11

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to hold sensor readings
float temp;
float hum;
float ShokeState;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings (10s)

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
  Serial.println("Connected to MqttBroker.");
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode (LedPin, OUTPUT) ;// define LED as output interface
  pinMode (ShockPin, INPUT) ;// define knock sensor output interface
  pinMode (BottonPin, INPUT) ;// define knock sensor output interface
  
  attachInterrupt(digitalPinToInterrupt(ShockPin), Drop, CHANGE); //shock interupt
  attachInterrupt(digitalPinToInterrupt(BottonPin), ChangeState, RISING); //Botton interupt

  dht.begin();
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { // it publishes a new MQTT message
    previousMillis = currentMillis; // Save the last time a new reading was published
    hum = dht.readHumidity(); // New DHT sensor readings
    temp = dht.readTemperature(); // Read temperature as Celsius (the default)
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //temp = dht.readTemperature(true);
    
    // Publish an MQTT message on topic Box1/Temp
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic Box1/Hum
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);

    // Publish an MQTT message on topic Box1/SS
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_SS, 1, true, String(ShokeState).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_SS, packetIdPub3);
    Serial.printf("Message: %.2f \n", ShokeState);
  }
}
void Drop() { //shock interupt
  Serial.println("interupt1"); 
  ShokeState = HIGH;
  digitalWrite(LedPin, HIGH);
}
void ChangeState() { //Butten interupt
  Serial.println("interupt2"); 
  ShokeState = LOW;
  digitalWrite(LedPin, LOW);
}
