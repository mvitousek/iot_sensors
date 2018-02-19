#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

#include "credentials.h"
// Defines HOME_SSID and PASS

// Required for LIGHT_SLEEP_T delay mode
//extern "C" {
//  #include "user_interface.h"
//}


#define DONE_PIN 2
#define TEMP_PIN 0  // DS18B20 pin
#define CONNECTION_ATTEMPTS 5
#define MIN_UP_TIME 3000

OneWire oneWire(TEMP_PIN);
DallasTemperature DS18B20(&oneWire);

#define SERVER "192.168.1.116"
#define PORT 8712
#define TIMEOUT_SECONDS 10
#define DELAY_TIME 500

unsigned long start_time;
uint8_t mac[WL_MAC_ADDR_LENGTH];
String mac_name;
float temp;
String post_data;

WiFiClient client;


void setup() {
  start_time = millis();
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, HIGH);
  Serial.begin(115200);
  WiFi.macAddress(mac);
  mac_name = String("");
  for (int i = 0; i < WL_MAC_ADDR_LENGTH; i++) {
    mac_name = mac_name + String(mac[i], HEX);
  }
  Serial.println(mac_name);
  
  connect_to_wifi();
}


void connect_to_wifi() {
  WiFi.begin(HOME_SSID, PASS);
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  Serial.print("Working to connect to ");
  Serial.print(HOME_SSID);
  Serial.print("...");

  for (int i = 0; i < TIMEOUT_SECONDS * 1000 / DELAY_TIME && (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_DISCONNECTED); i++) {
    delay(DELAY_TIME);
    Serial.print(".");
  }
  Serial.println(" done.");
  delay(5);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected.");
    delay(5);
    send_temp();
  } else {
    Serial.print("Connection failed with code ");
    Serial.println(WiFi.status());
    fail();
  }
}

void get_temp() {
  temp = 85;
  while (temp == 85 || temp == -127) {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
  }
  Serial.print("Temperature: ");
  Serial.println(temp);
  delay(5);
}

void send_temp() {
  get_temp();
  post_data = "mac=" + mac_name;
  post_data += "&temp=";
  post_data += temp;
  for (int i = 0; i < CONNECTION_ATTEMPTS; i++) {
    if (client.connect(SERVER, PORT)) {
      Serial.println("Sending update");
      delay(5);
      Serial.flush();
      client.println("POST /signal/temp HTTP/1.1");
      client.print("Host: ");
      client.println(SERVER);
      client.println("Cache-Control: no-cache");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(post_data.length());
      client.println();
      client.println(post_data);
      client.flush();
      client.stop();
      success();
      return;
    } else {
      Serial.println("Failed to connect to server");
      delay(500);
    }
  }
  fail();
}

void success() {
  unsigned long cur_time = millis();
  if (cur_time - start_time < MIN_UP_TIME) {
    delay(MIN_UP_TIME - (cur_time - start_time));
  }

  Serial.println("Successfully transmitted");
  delay(5);

  digitalWrite(DONE_PIN, LOW);
  delay(100);
  digitalWrite(DONE_PIN, HIGH);
  
}

void fail() {
  Serial.println("Failed to transmit");
  digitalWrite(DONE_PIN, LOW);
}

void loop() {
  #ifdef DEBUG
  Serial.println("RRT");
  delay(5000);
  ESP.restart();
  delay(1000);
  Serial.println("RST");
  ESP.reset();
  #endif
  // Nothing
}
