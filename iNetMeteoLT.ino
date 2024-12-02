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

/*
If you do an update you WILL have to edit the "User_Setup_Select.h" to comment-out the line:
"//#include <User_Setup.h> // Default setup is root library folder"
AND un-comment:
"#include <User_Setups/Setup206_LilyGo_T_Display_S3.h> // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT"

https://github.com/teastainGit/LilyGO-T-display-S3-setup-and-examples/blob/main/T-DisplayS3_Setup.txt
*/

/*
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
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
#define CONFIG_BUTTON 14
#define EEPROM_SIZE 25
#define CAPTIVE_TIMEOUT 120

int screenRotation = 1;

char MQTT_TOPIC[26];

unsigned long previous_time = 0;   //WiFi delay period start
unsigned long wifi_delay = 10000;  // 10 seconds delay for WiFi recoonect

double temp = 0;
double windspd = 0;
int winddir = 0;
char update[21] = { 0 };        //text with last update time
char station_name[14] = { 0 };  //text with station ID

WiFiClientSecure wifiClient = WiFiClientSecure();
MqttClient mqttClient(wifiClient);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite numberDash = TFT_eSprite(&tft);
TFT_eSprite directionDash = TFT_eSprite(&tft);

void setup() {
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  tft.init();

  //read rotation settings from EEPROM
  screenRotation = readRotation();

  tft.setRotation(screenRotation);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(FONT2);

  WiFi.mode(WIFI_STA);

  if (digitalRead(CONFIG_BUTTON) == LOW) {
    runCaptivePortal();
  }

  MQTT_TOPIC[0] = '\0';
  strcat(MQTT_TOPIC, "weather/");
  strcat(MQTT_TOPIC, readStationID());
  Serial.println(MQTT_TOPIC);

  //WiFiManager wifiManager;
  //wifiManager.setConfigPortalTimeout(CAPTIVE_TIMEOUT);
  //wifiManager.autoConnect("ConfigMeteo");

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
  delay(1000);
}

void loop() {

  if (digitalRead(CONFIG_BUTTON) == LOW) {
    rotateScreen();
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

  if (myArray.hasOwnProperty("id")) {
    strncpy(station_name, (const char *)myArray["id"], 14);
  }

  Serial.println(topicContent);
  Serial.println(update);
}

//respond to button interrupt and invert rotation value
void rotateScreen() {
  static uint32_t lastInt = 0;
  if (millis() - lastInt > 200) {
    screenRotation = -1 * screenRotation;
    writeRotation(screenRotation);
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
  tft.println("Config Mode");
  tft.print("Connect to the ConfigPortal WiFi");
  // id/name, placeholder/prompt, default, length
  WiFiManager wifiManager;
  //wifiManager.setCleanConnect(true);
  WiFiManagerParameter stationID("id", "Station ID", "0000", 24);
  wifiManager.setConfigPortalTimeout(CAPTIVE_TIMEOUT);
  wifiManager.addParameter(&stationID);
  wifiManager.startConfigPortal("ConfigPortal");
  writeStationID(stationID.getValue());

  MQTT_TOPIC[0] = '\0';
  strcat(MQTT_TOPIC, "weather/");
  strcat(MQTT_TOPIC, stationID.getValue());
  Serial.println(MQTT_TOPIC);

  Serial.println("connected");
  Serial.println("Exiting CAPTIVE function");
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
  int eeprom_location = 1;  //read from location 1, loc 0 for rotation entry
  int string_location = 0;
  while (EEPROM.read(eeprom_location) != 0 || eeprom_location < EEPROM_SIZE) {
    station_id[string_location] = EEPROM.read(eeprom_location);
    eeprom_location++;
    string_location++;
  }
  return station_id;
}

void writeRotation(int rotation) {
  if (rotation == 1 || rotation == -1) {
    int value = (rotation + 1) / 2;  //write 0 if rotation set to -1 and 1 if rotation set to 1
    EEPROM.write(0, value);
    EEPROM.commit();
  } else {
    EEPROM.write(0, 1);  //if supplied values are out of bounds, write 1
  }
}

int readRotation() {
  int value = EEPROM.read(0);
  if (value == 0 || value == 1) {  //somthing was written in EEPROM
    return 2 * value - 1;          //return 1 on EEPROM value 1 and -1 on value 0
  } else {                         //nothing in EEPROM, probabbly firs time run
    EEPROM.write(0, 1);
    EEPROM.commit();
    return 1;
  }
}