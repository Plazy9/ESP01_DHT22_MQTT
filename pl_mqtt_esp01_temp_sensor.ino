#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//inc. for Timecheck from NTP
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // UTC +1 (3600 másodperc), frissítés 60 másodpercenként

//temp sensor
#include <DHT.h>
#include <DHT_U.h>

#define MY_BLUE_LED_PIN 1

// DHT config.
#define DHTPIN            2         // Pin which is connected to the DHT sensor.
// Uncomment the type of sensor in use:
//#define DHTTYPE           DHT11     // DHT 11 
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);

float myTemperature = 0, myHumidity = 0; 

// Update these with values suitable for your network.

const char* ssid = wifiSSID;
const char* password = wifiPassword;
//const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* mqtt_server = mqttServer;
const char* mqttServerUser = mqttUser;
const char* mqttServerPWD = mqttPassword;

const String mqttMainTopic = mqttMainTopic_CFG;
const String mqttDeviceName = mqttDeviceName_CFG; 

String full_mqtt_topic = (mqttMainTopic+"/"+mqttDeviceName).c_str();


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  //pinMode(RELAY_PIN, OUTPUT);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  randomSeed(micros());

  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
  }
  //Serial.println();
/*
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    client.publish("pl_esp01_relay/pl_stateTopic", "on");
    digitalWrite(MY_BLUE_LED_PIN , LOW);   // Turn the LED on (Note that LOW is the voltage level
    digitalWrite(RELAY_PIN , LOW);
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    client.publish("pl_esp01_relay/pl_stateTopic", "off");
    digitalWrite(MY_BLUE_LED_PIN , HIGH);  // Turn the LED off by making the voltage HIGH
    digitalWrite(RELAY_PIN , HIGH);
  }
*/
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqttServerUser, mqttServerPWD, (full_mqtt_topic+"/status").c_str(), 0, true, "offline")) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      snprintf (msg, MSG_BUFFER_SIZE, "online");
      client.publish((full_mqtt_topic+"/status").c_str() , msg);

      // ... and resubscribe
      client.subscribe((full_mqtt_topic+"/commandTopic").c_str());
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(MY_BLUE_LED_PIN , OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(MY_BLUE_LED_PIN, LOW);
  
  timeClient.begin(); //NPT client
  dht.begin();

  //Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    //Led status change

    digitalWrite(MY_BLUE_LED_PIN, !digitalRead(MY_BLUE_LED_PIN));

    timeClient.update();
    //Serial.println(timeClient.getFormattedTime());

    sensors_event_t event;  
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      //Serial.println("Error reading temperature!");
    } else {
      // Update temperature and humidity
      myTemperature = (float)event.temperature;
      //Serial.print("Temperature: ");
      //Serial.print(myTemperature);
      //Serial.println(" C");
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      //Serial.println("Error reading humidity!");
    } else {
      myHumidity = (float)event.relative_humidity;
      //Serial.print("Humidity: ");
      //Serial.print(myHumidity);
      //Serial.println("%");
    }

    //++value;

    snprintf (msg, MSG_BUFFER_SIZE, "%.2lf", myTemperature);
    client.publish((full_mqtt_topic+"/temperature").c_str() , msg);

    snprintf (msg, MSG_BUFFER_SIZE, "%.2lf", myHumidity);
    client.publish((full_mqtt_topic+"/humidity").c_str() , msg);

  }
}
