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
#include "cert.h"

#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h"
#include "Dashboard.h"

#define ROTATE_BUTTON 14
int screenRotation = 1;

//const char MQTT_TOPIC[] = "weather/0000";
const char MQTT_TOPIC[] = "weather/0310";
//const char MQTT_TOPIC[] = "weather/1187";

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

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to AWS IoT
  wifiClient.setCACert(CERT_CA);
  mqttClient.setUsernamePassword(MQTT_USER, MQTT_PASS);
  if (mqttClient.connect(IOT_ENDPOINT, 8883)) {
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
  } else {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }

  // Subscribe to MQTT and register a callback
  mqttClient.onMessage(messageHandler);
  mqttClient.subscribe(MQTT_TOPIC);
}

void loop() {
  // put your main code here, to run repeatedly:
  mqttClient.poll();
  delay(10);
}

void messageHandler(int messageSize) {

  //char topicContent[messageSize] = {0};
  char topicContent[256] = {0};

  int i = 0;
  while (mqttClient.available()) {
    topicContent[i] = (char)mqttClient.read();
    i++;
  }
  //Terminate shar string with 0
  topicContent[i] = '\0';

  JSONVar myArray = JSON.parse(topicContent);

  Serial.println(messageSize);
  Serial.println(screenRotation);
  Serial.println(topicContent);
  /*
  Serial.print("Temperature:\t");
  Serial.println(myArray["temp"]);
  
  Serial.print("Windspeed:\t");
  Serial.println(myArray["windspd"]);

  Serial.print("Winddir:\t");
  Serial.println(myArray["winddir"]);

  Serial.print("Name:\t");
  Serial.println(myArray["station_name"]);
  */
  showDash(&numberDash, (double) myArray["temp"], (double) myArray["windspd"]);
  showDirection(&directionDash, &numberDash, (int) myArray["winddir"]);
  numberDash.pushSprite(0, 0);
}

void rotateScreen(){
  screenRotation = -1 * screenRotation;
  tft.setRotation(screenRotation);
  //tft.fillScreen(TFT_BLUE);
}
