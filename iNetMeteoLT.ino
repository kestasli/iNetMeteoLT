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

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include "cert.h"

#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h"
#include "Dashboard.h"

#define ROTATE_BUTTON 14
#define CONFIG_BUTTON 0
int screenRotation = 1;
bool wifiConfigMode = false;

//const char MQTT_TOPIC[] = "weather/0310";
//const char MQTT_TOPIC[] = "weather/0000";
//const char MQTT_TOPIC[] = "weather/7336";
//const char MQTT_TOPIC[] = "weather/1187";
//const char MQTT_TOPIC[] = "weather/4001";
char MQTT_TOPIC[24] = { 0 };

unsigned long previous_time = 0;   //WiFi delay period start
unsigned long wifi_delay = 10000;  // 10 seconds delay for WiFi recoonect

double temp = 0;
double windspd = 0;
int winddir = 0;
char update[21] = { 0 }; //this holds text with last update time

WiFiClientSecure wifiClient = WiFiClientSecure();
MqttClient mqttClient(wifiClient);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite numberDash = TFT_eSprite(&tft);
TFT_eSprite directionDash = TFT_eSprite(&tft);

void setup() {
  // initialize the serial port
  Serial.begin(115200);
  tft.init();
  tft.setRotation(screenRotation);
  tft.fillScreen(TFT_BLACK);

  pinMode(ROTATE_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ROTATE_BUTTON), rotateScreen, FALLING);

  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONFIG_BUTTON), enterConfig, FALLING);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  //Serial.println("Flash:");
  //Serial.println(stationID.getValue());

  WiFi.mode(WIFI_STA);

  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

  if (wifiConfigMode) {
    WiFiManager wifiManager;
    // id/name, placeholder/prompt, default, length
    WiFiManagerParameter stationID("id", "Station ID", "0000", 24);
    wifiManager.addParameter(&stationID);
    wifiManager.startConfigPortal("ConfigMeteo");

    MQTT_TOPIC[0] = '\0';
    strcat(MQTT_TOPIC, "weather/");
    strcat(MQTT_TOPIC, stationID.getValue());

    Serial.println("connected");
    Serial.println(MQTT_TOPIC);
    wifiConfigMode = false;
  }

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

  //Serial.println(messageSize);
  //Serial.println(screenRotation);
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

void writeStationID(char *station_id) {
  Serial.println("EEPROM write");
  unsigned int eeprom_location = 1; //write to location 1, loc 0 for rotation entry
  while (*station_id) {
    EEPROM.write(eeprom_location, *station_id);
    station_id++;
    eeprom_location++;
    if (eeprom_location > 23) {break;}
  }
  EEPROM.write(eeprom_location, 0); //add 0 at the end to indicate string stop
}

char* readStationID(){
  Serial.println("EEPROM read");
  static char station_id[24];
  int eeprom_location = 1;
  int string_location = 0;
  while (EEPROM.read(eeprom_location) != 0){
    station_id[string_location] = EEPROM.read(eeprom_location);
    //Serial.println(station_id);
    eeprom_location++;
    string_location++;
  }
  return station_id;
}

void writeRotation(bool rotate){
  EEPROM.write(0, rotate);
}

bool readRotation(){
  return EEPROM.read(0);
}
