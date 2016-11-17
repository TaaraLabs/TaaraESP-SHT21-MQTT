/* 
 * Send temperature and humidity data from SHT21/HTU21 sensor on ESP8266 to EmonCMS over HTTPS or HTTP
 * 
 * Demo for TaaraESP SHT21 Wifi Humidity sensor https://taaralabs.eu/es1
 * 
 * by Märt Maiste, TaaraLabs
 * https://github.com/TaaraLabs/TaaraESP-SHT21-EmonCMS
 * 2016-08-08
 *
*/

#include <ESP8266WiFi.h>       // https://github.com/esp8266/Arduino
#include <WiFiClientSecure.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <WiFiManager.h>       // https://github.com/tzapu/WiFiManager

#include "SparkFunHTU21D.h"    // https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library

#include <PubSubClient.h>      // https://github.com/knolleary/pubsubclient

#include <EEPROM.h>

//#define DEBUG 1          // serial port debugging
#define HTTPS 1          // use secure connection

const int interval  = 5; // send readings every 5 minutes
const int LED       = 13;
const int BUTTON    = 0; // program button is available after power-up for entering configPortal

struct CONFIG {          // config data is very easy to read and write in the EEPROM using struct
  char host[40];         // MQTT Server name
  char topic[32];        // MQTT Topic
  char user[32];         // MQTT Username
  char pass[32];         // MQTT Password
  byte checksum;         // Calculating real CRC is too hard
};
const byte checksum = 73;

CONFIG conf;             // initialize conf struct

bool save = false;       // WifiManager will not change the config data directly
void saveConfig () {     // it uses callback function instead
  save = true;           // mark down that something has been changed via WifiManager
}

// blink the LED shortly for number of times, wait a second and repeat for 5 minutes
void blink (byte times) {
  unsigned long start = millis();
  while (millis() - start < 5 * 60 * 1000) {
    for (byte i=0; i < times; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
    if (times < 5) {  // if too many blinks in a row then
      delay(1000);    // just keep blinking without delays
    }
  }  
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("\n\nTaaraESP SHT21 [taaralabs.eu/es1]\n");
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // turn the led on before taking new measurement and connecting to wifi AP

  HTU21D sht;
  Wire.begin(2,14);        // overriding the i2c pins
  //sht.begin();           // default: sda=4, scl=5
  float humidity    = sht.readHumidity();
  float temperature = sht.readTemperature();

  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);

  #ifdef DEBUG
  Serial.print("ESP chip ID: ");
  Serial.println(ESP.getChipId());
  #ifdef HTTPS
  Serial.println("Using secure connection");
  #endif
  #endif

  EEPROM.begin(200); // using flash for config data
  EEPROM.get<CONFIG>(0, conf);

  if (conf.checksum != checksum) {
    #ifdef DEBUG
    Serial.println("The checksum does not match, clearing data");
    #endif    
    memset(conf.host,   0, sizeof(conf.host));
    memset(conf.topic,  0, sizeof(conf.topic));
    memset(conf.user,   0, sizeof(conf.user));
    memset(conf.pass,   0, sizeof(conf.pass));
  }
  
  #ifdef DEBUG
  Serial.print("host: ");   Serial.println(conf.host);
  Serial.print("topic: ");  Serial.println(conf.topic);
  Serial.print("user: ");   Serial.println(conf.user);
  Serial.print("pass: ");   Serial.println(conf.pass);
  #endif

  WiFiManagerParameter custom_host("conf.host",   "MQTT server", conf.host,  40);
  WiFiManagerParameter custom_topic("conf.topic", "MQTT topic",  conf.topic, 32);
  WiFiManagerParameter custom_user("conf.user",   "MQTT user",   conf.user,  32);
  WiFiManagerParameter custom_pass("conf.pass",   "MQTT pass",   conf.pass,  32);
  
  WiFiManager wifiManager;
  #ifndef DEBUG
  wifiManager.setDebugOutput(false);
  #endif
  wifiManager.setTimeout(300); // if wifi is not configured or not accessible then wait 5 minutes and then move on
  wifiManager.setSaveConfigCallback(saveConfig);
  wifiManager.addParameter(&custom_host);
  wifiManager.addParameter(&custom_topic);
  wifiManager.addParameter(&custom_user);
  wifiManager.addParameter(&custom_pass);


  pinMode(BUTTON, INPUT);

  #ifdef DEBUG
  Serial.println("Wait a sec for config button");
  #endif
  unsigned long start = millis();

  while (millis() - start < 1000) {                    // check if config button was pressed in one second after power up
    if (digitalRead(BUTTON) == LOW) {
      Serial.println("Config button was pressed");
      wifiManager.setTimeout(0);                       // no timeout if the button was pressed
      wifiManager.startConfigPortal();                 // start the config portal
      break;
      }
    delay(100);
    }
  #ifdef DEBUG
  Serial.println("Config button was not pressed");
  #endif

  if (save == true) {                                       // if save flag is set by the callback function of the wifiManager
    Serial.println("Saving config");
    strcpy(conf.host,  custom_host.getValue());            // read the data from wifiManager registers
    strcpy(conf.topic, custom_topic.getValue());
    strcpy(conf.user,  custom_user.getValue());
    strcpy(conf.pass,  custom_pass.getValue());
    conf.checksum = checksum;
    
    #ifdef DEBUG
    Serial.print("host: ");  Serial.println(conf.host);
    Serial.print("topic: "); Serial.println(conf.topic);
    Serial.print("user: ");  Serial.println(conf.user);
    Serial.print("pass: ");  Serial.println(conf.pass);
    #endif
    EEPROM.put<CONFIG>(0, conf);                            // write the config data to the flash
    EEPROM.commit();
    delay(10);
  }
  
  if (wifiManager.autoConnect()) { // try to connect to wifi AP using stored credentials
  
    digitalWrite(LED, LOW);        // if wifi connection is established then the led goes out (or stays on if there is a problem with wifi connection)

    #ifdef HTTPS
    WiFiClientSecure client;
    const int port = 8883;
    #else
    WiFiClient client;
    const int port = 1883;
    #endif
    
    #ifdef DEBUG  
    Serial.print("Connecting to ");
    Serial.print(conf.host);
    Serial.print(":");
    Serial.println(port);
    #endif

    PubSubClient mqtt(client);
    mqtt.setServer(conf.host, port);
    if (!mqtt.connect("TaaraESP-SHT21", conf.user, conf.pass)) {
      #ifdef DEBUG
      Serial.println("Connection failed");
      #endif
      blink(1);    // blink 1 time, wait a second and repeat for 5 minutes
      ESP.reset(); // reset the board
      delay(5000); // resetting may take some time
    }

    #ifdef DEBUG
    Serial.println("Publishing data");
    #endif

    String topic_temperature = String(conf.topic) + PSTR("/temperature");
    String topic_humidity    = String(conf.topic) + PSTR("/humidity");

    // try to publish and if it failes, blink 2 times, wait a second and repeat for 5 minutes
    if (!mqtt.publish(topic_temperature.c_str(), String(temperature).c_str())) {

      #ifdef DEBUG
      Serial.println("Publishing failed");
      #endif
      
      blink(2);            
      ESP.reset();
      delay(5000);
    }
    
    if (!mqtt.publish(topic_humidity.c_str(), String(humidity).c_str())) {

      #ifdef DEBUG
      Serial.println("Publishing failed");
      #endif

      blink(2);
      ESP.reset();
      delay(5000);
    }
  }
  
  Serial.print("Going to sleep for ");
  Serial.print(interval);
  Serial.println(" minutes");
  delay(3000);                            // let the module to get everything in order in the background
  ESP.deepSleep(interval * 60 * 1000000); // deep sleep means turning off everything except the wakeup timer. wakeup call resets the chip
  delay(5000);                            // going to sleep may take some time
}

void loop() {                             // we should never reach this point, but if we do, then 
  blink(10);                              // blink continuously for five minutes
  delay(3000);
  ESP.reset();                            // and reset the module
  delay(5000);  
}
