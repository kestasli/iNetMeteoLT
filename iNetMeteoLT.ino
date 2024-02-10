/*
USB CDC On Boot 	  Enabled
CPU Frequency   	  240MHz (WiFi)
Core Debug Level 	  None
USB DFU On Boot	    Enabled
Events Run On	Core  1
Flash Mode	        QIO 80MHz
Flash Size	        16MB (128Mb)
JTAG Adapter	      Integrated USB JTAG
Arduino Runs On	    Core 1
USB Firmware MSC On Boot	Disabled
Partition Scheme	  Huge APP (3MB No OTA/1MB SPIFFS)
PSRAM	              OPI PSRAM
USB Mode	          Hardware CDC and JTAG
*/

#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>

#include "Free_Fonts.h"
#include "Dashboard.h"
#include "cert.h"

#define ROTATE_BUTTON 14
#define CONFIG_BUTTON 0
#define EEPROM_SIZE 25
#define CAPTIVE_TIMEOUT 120

int screenRotation = 1;
bool wifiConfigMode = false;

char MQTT_TOPIC[26];

unsigned long previous_time = 0;   //WiFi delay period start
unsigned long wifi_delay = 10000;  // 10 seconds delay for WiFi recoonect

double temp = 0;
double windspd = 0;
int winddir = 0;
char update[21] = { 0 };  //this holds text with last update time

WiFiClientSecure wifiClient = WiFiClientSecure();
MqttClient mqttClient(wifiClient);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite numberDash = TFT_eSprite(&tft);
TFT_eSprite directionDash = TFT_eSprite(&tft);

void setup() {
  // initialize the serial port
  Serial.begin(115200);
  Serial.print("Config mode: ");
  Serial.println(wifiConfigMode);

  EEPROM.begin(EEPROM_SIZE);
  tft.init();

  tft.setRotation(screenRotation);
  tft.fillScreen(TFT_BLACK);

  pinMode(ROTATE_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ROTATE_BUTTON), rotateScreen, FALLING);

  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONFIG_BUTTON), enterConfig, FALLING);

  delay(1000);

  //MQTT_TOPIC[0] = '\0';

  strcat(MQTT_TOPIC, "weather/");
  strcat(MQTT_TOPIC, readStationID());
  Serial.println(MQTT_TOPIC);

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());

  // Connect to HiveMQ IoT
  wifiClient.setCACert(CERT_CA);
  connectHiveMQ(&mqttClient);
}

void loop() {

  if (wifiConfigMode) { runCaptivePortal(); }

  unsigned long current_time = millis();  // number of milliseconds since the upload

  // checking for WIFI connection, reconnect if neded
  if ((WiFi.status() != WL_CONNECTED) && (current_time - previous_time >= wifi_delay)) {
    Serial.println("Reconnecting to WIFI network");
    WiFi.disconnect();
    WiFi.reconnect();
    previous_time = current_time;
  }

  if (mqttClient.connected()) {
    //Do server polling a
    mqttClient.poll();
    tft.setRotation(screenRotation);
    showDash(&numberDash, temp, windspd, update);
    showDirection(&directionDash, &numberDash, winddir);
    numberDash.pushSprite(0, 0);

  } else {
    if (current_time - previous_time >= wifi_delay) {
      //Retry connection if server lost
      Serial.println("MQTT connection down");
      //delay(1000);  //should replace not to disrupt loop
      connectHiveMQ(&mqttClient);
      previous_time = current_time;
    }
  }
}

//---------------FUNCTIONS---------------

void messageHandler(int messageSize) {
  char topicContent[256] = { 0 };
  int i = 0;
  while (mqttClient.available()) {
    topicContent[i] = (char)mqttClient.read();
    i++;
  }
  //Terminate shar string with 0
  topicContent[i] = '\0';
  JSONVar myArray = JSON.parse(topicContent);

  if (myArray.hasOwnProperty("temp")) {
    temp = (double)myArray["temp"];
  }

  if (myArray.hasOwnProperty("spd")) {
    windspd = (double)myArray["spd"];
  }

  if (myArray.hasOwnProperty("dir")) {
    winddir = (int)myArray["dir"];
  }

  if (myArray.hasOwnProperty("update")) {
    strncpy(update, (const char *)myArray["update"], 21);
  }

  Serial.println(topicContent);
  Serial.println(update);
}

//respond to button interrupt and invert rotation value
void rotateScreen() {
  static uint32_t lastInt = 0;
  if (millis() - lastInt > 200) {
    screenRotation = -1 * screenRotation;
    lastInt = millis();
  }
}

//Handle button press interrupt and enter WiFi config mode
void enterConfig() {
  static uint32_t lastInt = 0;
  if (millis() - lastInt > 200) {
    wifiConfigMode = true;
    lastInt = millis();
  }
}

//handle connection to MQTT server
void connectHiveMQ(MqttClient *client) {
  // Connect to HiveMQ IoT
  client->setUsernamePassword(MQTT_USER, MQTT_PASS);

  if (client->connect(IOT_ENDPOINT, 8883)) {
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
  } else {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }

  // Subscribe to MQTT and register a callback
  client->onMessage(messageHandler);
  client->subscribe(MQTT_TOPIC);
}


void runCaptivePortal() {
  Serial.println("CAPTIVE START");
  WiFiManager wifiManager;
  //wifiManager.setConfigPortalTimeout(CAPTIVE_TIMEOUT);
  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter stationID("id", "Station ID", "0000", 24);
  wifiManager.addParameter(&stationID);
  wifiManager.startConfigPortal("ConfigMeteo");
  writeStationID(stationID.getValue());

  MQTT_TOPIC[0] = '\0';
  strcat(MQTT_TOPIC, "weather/");
  strcat(MQTT_TOPIC, stationID.getValue());
  Serial.println(MQTT_TOPIC);

  Serial.println("connected");
  wifiConfigMode = false;
}

void writeStationID(const char *station_id) {
  Serial.println("EEPROM write");
  unsigned int eeprom_location = 1;  //write to location 1, loc 0 for rotation entry
  while (*station_id || eeprom_location < EEPROM_SIZE) {
    EEPROM.write(eeprom_location, *station_id);
    station_id++;
    eeprom_location++;
  }
  EEPROM.commit();
}

char *readStationID() {
  static char station_id[EEPROM_SIZE];
  Serial.println("EEPROM read");
  int eeprom_location = 1;
  int string_location = 0;
  while (EEPROM.read(eeprom_location) != 0 || eeprom_location < EEPROM_SIZE) {
    station_id[string_location] = EEPROM.read(eeprom_location);
    eeprom_location++;
    string_location++;
  }
  return station_id;
}

void writeRotation(bool rotate) {
  EEPROM.write(0, rotate);
}

bool readRotation() {
  return EEPROM.read(0);
}