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

#define COMM_PIN 2
#define TEMP_PIN 0  // DS18B20 pin

OneWire oneWire(TEMP_PIN);
DallasTemperature DS18B20(&oneWire);

#define NO_WIFI      0
#define NO_SERVER    1
#define CLIENT       2
#define STARTUP      3

#define SERVER "192.168.1.116"
#define PORT 8712
#define TIMEOUT_SECONDS 30
#define RECONNECT_SECONDS 5
#define DELAY_TIME 500

unsigned long last_time;
int mode = STARTUP;
uint8_t mac[WL_MAC_ADDR_LENGTH];
String mac_name;
String post_data;
float temp;

WiFiClient client;


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

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected.");
    register_with_server();
  } else {
    Serial.print("Connection failed with code ");
    Serial.println(WiFi.status());
    WiFi.disconnect(true);
    mode = NO_WIFI;
  }
}

void register_with_server() {
  Serial.print("Registering with ");
  Serial.println(SERVER);
  
  if (client.connect(SERVER, PORT)) {
    Serial.println("Connected with server");
    Serial.flush();
    mode = CLIENT;
    client.stop();
  } else {
    Serial.println("Failed to connect to server");
    last_time = millis();
    mode = NO_SERVER;
  }
}

void setup() {
  last_time = millis();
  pinMode(COMM_PIN, OUTPUT);
  Serial.begin(115200);
  //WiFi.mode(WIFI_STA);
  delay(3000);
  WiFi.macAddress(mac);
  mac_name = String("");
  for (int i = 0; i < WL_MAC_ADDR_LENGTH; i++) {
    mac_name = mac_name + String(mac[i], HEX);
  }
  Serial.println(mac_name);
  
  connect_to_wifi();
}

void loop() {
  if (mode != NO_WIFI && WiFi.status() != WL_CONNECTED) {
    Serial.print("Lost connection with code ");
    Serial.println(WiFi.status());
    mode = NO_WIFI;
    delay(1000);
    
  } else if (mode == NO_WIFI || mode == NO_SERVER) {
    Serial.println("Attempting to reconnect");
    if (mode == NO_WIFI) {
      mode = STARTUP;
      connect_to_wifi();
    } else if (mode == NO_SERVER) {
      mode = STARTUP;
      register_with_server();
    }
    
    if (mode == CLIENT) {
      loop();
    } else {
      delay(RECONNECT_SECONDS * 1000);
    }
    
  } else if (mode == CLIENT) {
    send_update();
    delay(3000);
    
  } else {
    Serial.print("Unexpected mode ");
    Serial.println(mode);
  }
}

void get_temp() {
  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0);
  Serial.print("Temperature: ");
  Serial.println(temp);
}

void send_update() {
  get_temp();
  post_data = "mac=" + mac_name;
  post_data += "&temp=";
  post_data += temp;
  if (client.connect(SERVER, PORT)) {
    digitalWrite(COMM_PIN, LOW);
    Serial.println("Sending update");
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
    digitalWrite(COMM_PIN, HIGH);
  } else {
    Serial.println("Failed to connect to server");
    mode = NO_SERVER;
    digitalWrite(COMM_PIN, LOW);
  }
}
