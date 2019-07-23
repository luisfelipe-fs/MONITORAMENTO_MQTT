#include "ESP8266WiFi.h"
#include "PubSubClient.h"

#define UNIQUE_NAME "ECP-LAB1" // Hostname, must be unique

const char* BROKER = "broker.hivemq.com";
const int PORT = 1883;
const char* CLIENT_NAME = UNIQUE_NAME;
const char* CONTROL_TOPIC = "CONTROL/" UNIQUE_NAME; // Receive topic
const char* DATA_TOPIC = "MASTER/DATA"; // Send topic

// Choose a valid WPA2 authentication method
#define PERSONAL

// Authentication method = WPA2 Personal
#ifdef PERSONAL
const char* WIFI_SSID = "REDE";
const char* WIFI_PASSWORD = "REDEREDE";
#endif

// Authentication method = WPA2 Enterprise
#ifdef ENTERPRISE
static const char* WIFI_SSID = "ssid";
static const char* WIFI_USERNAME = "username";
static const char* WIFI_PASSWORD = "password";
extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}
#endif

WiFiClient esp8266_client;
PubSubClient client(esp8266_client);
char client_id = '#';

// Pins and states
const byte LIGHT_PIN = 16;
byte light_state = LOW;

const byte AIR_PIN = 5;
byte air_state = LOW;

const byte PIR_PIN = 4;
const int PIR_TIMEOUT = 10;
byte pir_state = LOW;
long pir_counter = 0;

byte automatic = LOW;

void connect2wifi () {
  #if defined(PERSONAL)
    WiFi.mode(WIFI_STA);
    WiFi.hostname(CLIENT_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #elif defined(ENTERPRISE)
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
      client.subscribe(CONTROL_TOPIC, 1); // Receive from
      client.publish(DATA_TOPIC, "#"); // Send to, Hello message
    }
    else {
      Serial.println("Failed. Trying again in 5s.");
      delay(5000);
    }
  }
  delay(500);
}

void callback (char* topic, byte* payload, unsigned int len) {
  *((char*) payload+len) = '\0';
  Serial.println("");
  Serial.print(topic);
  Serial.print(":");
  Serial.println((char*) payload);
  process_command(payload, len);
}

byte digital_set (char option, byte* state, byte pin) {
  if (option == '0') {
    *state = LOW;
    digitalWrite(pin, LOW);
  }
  else if (option == '1') {
    *state = HIGH;
    digitalWrite(pin, HIGH);
  }
  else {
    return 1; // Failure
  }
  return 0; // OK
}

void process_command (byte* payload, unsigned int len) {
  char id = (len > 0)?*((char*) payload):'#';
  byte cpos = 1; // Cursor's pos
  String answer = String(id);
  while (len > cpos) {
    char device = (len > cpos)?*((char*) payload+cpos):'#';
    char command = (len > cpos+1)?*((char*) payload+cpos+1):'#';
    byte increment = 2; // Shift cpos to
    switch (device) {
      case 'L': case 'l': // Light
        if (command == 'G' || command == 'g')
          answer = answer+device+command+(light_state?'1':'0');
        else if (command == 'S' || command == 's') {
          char option = (len > cpos+2)?*((char*) payload+cpos+2):'#';
          byte code = digital_set(option, &light_state, LIGHT_PIN);
          answer = answer+device+command+(code?'#':option);
          increment += 1;
        }
        else
          answer = answer+device+'#';
      break;
      case 'A': case 'a': // Air
        if (command == 'G' || command == 'g')
          answer = answer+device+command+(air_state?'1':'0');
        else if (command == 'S' || command == 's') {
          char option = (len > cpos+2)?*((char*) payload+cpos+2):'#';
          byte code = digital_set(option, &air_state, AIR_PIN);
          answer = answer+device+command+(code?'#':option);
          increment += 1;
        }
        else
          answer = answer+device+'#';
      break;
      case 'P': case 'p': // PIR sensor
        if (command == 'G' || command == 'g')
          answer = answer+device+command+(pir_state?'1':'0');
        else
          answer = answer+device+'#';
      break;
      case 'U': case 'u': // Automatic
        if (command == 'G' || command == 'g')
          answer = answer+device+command+(automatic?'1':'0');
        else if (command == 'S' || command == 's') {
          char option = (len > cpos+2)?*((char*) payload+cpos+2):'#';
          if (option == '0')
              automatic = LOW;
          else if (option == '1')
            automatic = HIGH;
          else
            option = '#';
          answer = answer+device+command+option;
          increment += 1;
        }
        else
          answer = answer+device+'#';
      break;
      default:
        answer = answer+'#';
      break;
    }
    cpos += increment;
  }
  client_id = id;
  client.publish(DATA_TOPIC, answer.c_str());
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
