// derived from the Basic MQTT example https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_basic/mqtt_basic.ino
/*
 * 
 * Sampled from pushing the button on the internet
 * 0xA0
 * 0x04
 * 0xxx
 * 0xA1
 * 
 * where 0xxx seems to be:
 * 0x00 both off
 * 0x01 relay one on
 * 0x02 relay two on
 * 0x03 both relays on
 * 
 * When compiling for the Sonoff dual Use :
 * Generic ESP8266
 * 1M 64K SPIFFS
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define LED 13

#define LED_ON 0
#define LED_OFF 1


// Update these with values suitable for your network.


const char* ssid = "******";
const char* password = "******";
const char* mqtt_server = "192.168.1.1";

boolean pump = false;
boolean boiler = false;

const unsigned long tUpdateLED  = 10000;

unsigned long tLEDFLASH;

#define LEDFLASHMILLIS 10000


WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
 
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    FlashLED();


    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  boolean bset = false;
  payload[length] = '\0';
  String sPayload = String((char *)payload);
  String sTopic = String(topic);
  if (sTopic == "house/heatingController/pump/set") {
    FlashLED();
    if (sPayload == "1") {
      if (pump == false) bset = true;
      pump = true;
    } else {
      if (pump) bset = true;
      pump = false;
    }
  }
  if (sTopic == "house/heatingController/boiler/set") {
    FlashLED();
    FlashLED();
    if (sPayload == "1") {
      if (boiler == false) bset = true;
      boiler = true;
    } else {
      if (boiler) bset = true;
      boiler = false;
    }
  }

  if (sTopic == "house/heatingController/heating/set") {
    FlashLED();
    FlashLED();
    FlashLED();
    if (sPayload == "1") {
      bset = true;
      pump = true;
      boiler = true;
    } else {
      bset = true;
      pump = false;
      boiler = false;
    }
  }
  
  if (bset) setrelays();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "sonoff-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      client.publish("emon/heatingController/connected", "1");
      setrelays();
      // ... and resubscribe
      client.subscribe("house/heatingController/#");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(19200);
  pinMode(LED, OUTPUT);

  FlashLED();

  tLEDFLASH = millis();


  setup_wifi();


// Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("HeatingController");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    FlashLED();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.loop();
  
  Serial.println("Ready");

}


void loop() {


  ArduinoOTA.handle();

  if ((millis() - tLEDFLASH) > tUpdateLED ) Heartbeat();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setrelays() {
  byte b = 0;
  if (pump) b++;
  if (boiler) b += 2;
  Serial.write(0xA0);
  Serial.write(0x04);
  Serial.write(b);
  Serial.write(0xA1);
  Serial.flush();
  if (pump) client.publish("emon/heatingController/pump", "1");
  else client.publish("emon/heatingController/pump", "0");

  if (boiler) client.publish("emon/heatingController/boiler", "1");
  else client.publish("emon/heatingController/boiler", "0");
}

void Heartbeat() {
  int rssi = WiFi.RSSI();
  char rssic[5];
  itoa(rssi, rssic, 10);
  client.publish("emon/heatingController/RSSI", rssic);
  if (pump) client.publish("emon/heatingController/pump", "1");
  else client.publish("emon/heatingController/pump", "0");

  if (boiler) client.publish("emon/heatingController/boiler", "1");
  else client.publish("emon/heatingController/boiler", "0");
  FlashLED();
}

void FlashLED() {

  Serial.println("FlashLED triggered");
  digitalWrite(LED, LED_ON);
  delay(50);
  digitalWrite(LED, LED_OFF);
  delay(20);

  tLEDFLASH = millis();
} 

void fastFlashLED() {

  Serial.println("FlashLED triggered");
  digitalWrite(LED, LED_ON);
  delay(50);
  digitalWrite(LED, LED_OFF);
  delay(20);

  //tLEDFLASH = millis();
} 
