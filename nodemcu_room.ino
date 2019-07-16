#include "ESP8266WiFi.h"
#include "PubSubClient.h"

// WPA2 AUTHENTICATION METHOD
#define PERSONAL

#ifdef ENTERPRISE
extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}
#endif

#define UNIQUE_NAME "ECP-LAB1"

const char* BROKER = "iot.eclipse.org";
const int PORT = 1883;
const char* CLIENT_NAME = UNIQUE_NAME;
const char* CONTROL_TOPIC = "CONTROL/" UNIQUE_NAME;
const char* DATA_TOPIC = "MASTER/DATA";

WiFiClient esp8266_client;
PubSubClient client(esp8266_client);
char client_id = '#';

// WPA2 Personal
#ifdef PERSONAL
const char* WIFI_SSID = "REDE";
const char* WIFI_PASSWORD = "REDEREDE";
#endif

// WPA2 Enterprise
#ifdef ENTERPRISE
static const char* WIFI_SSID = "UFMA";
static const char* WIFI_USERNAME = "luis.soares";
static const char* WIFI_PASSWORD = "password";
#endif

const byte LIGHT_PIN = 16;
const byte AIR_PIN = 5;
const byte PIR_PIN = 4;
const int PIR_TIMEOUT = 10;

byte state;
byte automatic = LOW;
byte light_state = LOW;
byte air_state = LOW;
byte pir_state = LOW;
long pir_counter = 0;

void connect2wifi () {
  #ifdef PERSONAL
    WiFi.mode(WIFI_STA);
    WiFi.hostname(CLIENT_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #elif ENTERPRISE
    struct station_config wifi_config;
    wifi_set_opmode(STATION_MODE);
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char*) wifi_config.ssid, WIFI_SSID);
    wifi_station_set_config(&wifi_config);
    wifi_station_clear_cert_key();
    wifi_station_clear_enterprise_ca_cert();
    wifi_station_set_wpa2_enterprise_auth(1);
    wifi_station_set_enterprise_identity((uint8*) WIFI_USERNAME, strlen(WIFI_USERNAME));
    wifi_station_set_enterprise_username((uint8*) WIFI_USERNAME, strlen(WIFI_USERNAME));
    wifi_station_set_enterprise_password((uint8*) WIFI_PASSWORD, strlen(WIFI_PASSWORD));
    wifi_station_connect();
  #else
    return;
  #endif

  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (!WiFi.isConnected()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. Host: ");
  Serial.println(WiFi.localIP());
}

void connect2broker () {
  client.setServer(BROKER, PORT);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println();
    Serial.print("Trying to reach MQTT Broker: ");
    Serial.println(BROKER);
    if (client.connect(CLIENT_NAME)) {
      Serial.print("Connected. Control topic: ");
      Serial.println(CONTROL_TOPIC);
      client.subscribe(CONTROL_TOPIC);
      client.publish(DATA_TOPIC, "Hello");
    }
    else {
      Serial.println("Failed. Trying again in 5s.");
      delay(5000);
    }
  }
  delay(500);
}

void callback (char* topic, byte* payload, unsigned int len) {
  ((char*) payload)[len] = '\0';
  Serial.println("");
  Serial.print(topic);
  Serial.print(":");
  Serial.println((char*) payload);
  process_command(payload, len);
}

void process_command (byte* payload, unsigned int len) {
  char id = (len > 0)?(char) payload[0]:'#';
  char command = (len > 1)?(char) payload[1]:'#';
  char device = (len > 2)?(char) payload[2]:'#';
  char option = (len > 3)?(char) payload[3]:'#';
  char data[5] = {id, command, device, option};
  switch (command) {
    case 'G':
      if (device == 'L') // LIGHT
        data[3] = light_state?'1':'0';
      else if (device == 'A') // AIR
        data[3] = air_state?'1':'0';
      else if (device == 'P') // PIR
        data[3] = pir_state?'1':'0';
      else if (device == 'S') // STATION
        data[3] = automatic?'1':'0';
      else // ERROR
        data[2] = '#';
    break;
    case 'S':
      if (device == 'L') { // LIGHT
        if (option == '0') {
          light_state = LOW;
          digitalWrite(LIGHT_PIN, LOW);
        }
        else if (option == '1') {
          light_state = HIGH;
          digitalWrite(LIGHT_PIN, HIGH);
        }
        else
          data[3] = '#';
      }
      else if (device == 'A') { // AIR
        if (option == '0') {
          air_state = LOW;
          digitalWrite(AIR_PIN, LOW);
        }
        else if (option == '1') {
          air_state = HIGH;
          digitalWrite(AIR_PIN, HIGH);
        }
        else // ERROR
          data[3] = '#';
      }
      else if (device == 'S') { // STATION
        if (option == '0')
          automatic = LOW;
        else if (option == '1')
          automatic = HIGH;
        else // ERROR
          data[3] = '#';
      }
      else // ERROR
        data[2] = '#';
    break;
    default: // ERROR
      data[1] = '#'; 
    break;
  }
  client_id = id;
  client.publish(DATA_TOPIC, data);
  delay(500);
}

void check_pir () {
  byte state = digitalRead(PIR_PIN);
  if (state) {
    pir_counter = millis();
    if (pir_state != HIGH) {
      pir_state = HIGH;
      if (automatic) {
        light_state = HIGH;
        digitalWrite(LIGHT_PIN, HIGH);
        air_state = HIGH;
        digitalWrite(AIR_PIN, HIGH);
        if (client_id != '#') {
          char data[5] = {client_id, 'G', 'L', '1'};
          client.publish(DATA_TOPIC, data);
          data[2] = 'A';
          client.publish(DATA_TOPIC, data);
        }
      }
      if (client_id != '#') {
        char data[5] = {client_id, 'G', 'P', '1'};
        client.publish(DATA_TOPIC, data);
        delay(500);
      }
    }
  }
  if (millis() - pir_counter > 1000*PIR_TIMEOUT)
    if (pir_state != LOW) {
      pir_state = LOW;
      if (automatic) {
        light_state = LOW;
        digitalWrite(LIGHT_PIN, LOW);
        air_state = LOW;
        digitalWrite(AIR_PIN, LOW);
        if (client_id != '#') {
          char data[5] = {client_id, 'G', 'L', '0'};
          client.publish(DATA_TOPIC, data);
          data[2] = 'A';
          client.publish(DATA_TOPIC, data);
        }
      }
      if (client_id != '#') {
        char data[5] = {client_id, 'G', 'P', '0'};
        client.publish(DATA_TOPIC, data);
        delay(500);
      }
    }
}

void setup () {
  Serial.begin(112500);
  Serial.println();
  pinMode(PIR_PIN, INPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, light_state);
  pinMode(AIR_PIN, OUTPUT);
  digitalWrite(AIR_PIN, air_state);
  
  connect2wifi();
  connect2broker();
}

void loop () {
  if (!client.connected())
    connect2broker();
  
  check_pir();
  client.loop();
  delay(50);
}
