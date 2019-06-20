/*
   Serra automatizzata

   MakeItModena

*/
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <SimpleDHT.h> // https://github.com/winlinvip/SimpleDHT
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define pinDHT22 D0
#define pinSoil A0
#define pinPomp D8
unsigned long lastMsg;
#define refreshTime 10000 // seconds
SimpleDHT22 dht22(pinDHT22);

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "xx"
#define WLAN_PASS       "yy"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "aaa"
#define AIO_KEY         "bbb"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish soil = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/soil");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pomp");

/***************************************************************************/

//
// --- DHT ---
//
void dht(byte *temp, byte *hum) {
  byte temperature;
  byte humidity;

  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err); delay(2000);
    return;
  }

  *temp = temperature;
  *hum = humidity;

  Serial.print(temperature); Serial.print(" *C, ");
  Serial.print(humidity); Serial.println(" H");


}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}


//
// --- SETUP ---
//
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}


//
// --- LOOP ---
//
void loop() {
  byte temp = 0;
  byte hum = 0;
  int valSoil = 0;

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // reads stuffs
  if (millis() - lastMsg > refreshTime) {
    lastMsg = millis();

    // read dht11
    dht(&temp, &hum);

    // read hum soil
    valSoil = analogRead(pinSoil);
    valSoil = valSoil / 4; // for better visibility on scale
    Serial.print("Hub Soil: ");  Serial.println(valSoil);
    
    // start water pomp for a second if valSoil misure is <= x
    if (valSoil <= soglia_critica)
      digitalWrite(pinPomp, HIGH); //water pomp on
      delay(1000); //wait 1 second
      digitalWrite(pinPomp, LOW); //water pomp off
    else
      digitalWrite(pinPomp, LOW); //water pomp off

  Serial.print(F("\nSending temperature val "));
  Serial.print(temp);
  Serial.print("...");
  if (! temperature.publish(temp)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }


  Serial.print(F("\nSending humidity val "));
  Serial.print(hum);
  Serial.print("...");
  if (! humidity.publish(hum)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }


  Serial.print(F("\nSending soil val "));
  Serial.print(valSoil);
  Serial.print("...");
  if (! soil.publish(valSoil)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

}
}
