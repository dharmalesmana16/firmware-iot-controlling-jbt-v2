#include <Arduino_MQTT_Client.h>
#include <ArduinoJson.h>
#include <ThingsBoard.h>
#include <Ethernet.h>
#include <SPI.h>
#include <EEPROM.h>
#include <PZEM004Tv30.h>
#include <Update.h>
#include <RTClib.h>
// #include <WiFi.h>
#include <EEPROM.h>
#define RXD1 18
#define TXD1 17
#define NUM_PZEMS 3
PZEM004Tv30 pzems[NUM_PZEMS];
IPAddress ip(192, 168, 1, 61);
IPAddress gw(192, 168, 1, 1);
IPAddress dns(8, 8, 8, 8);
IPAddress subnet(255, 255, 255, 0);
char buffer[12];
byte mac[] = {
  0x34, 0x85, 0x18, 0x46, 0x74, 0x80
};
// byte mac[] = { 0xcc, 0xdb, 0xa7, 0x15, 0x1f, 0x4c };

// 34:85:18:46:74:80
constexpr char TOKEN[] = "cm-pullruas";  //local uturnakame
constexpr char THINGSBOARD_SERVER[] = "43.252.159.216";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
int photoCellOFF = 0;
int photoCellON = 0;
int statusPC = 0;
char attributes[1000];
char msg[300];
RTC_DS3231 rtc;
constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";
volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;
const int RelayRA = 1;
const int RelaySA = 2;
const int RelayTA = 5;
const int RelayRB = 6;
const int RelaySB = 7;
const int RelayTB = 15;
String method;
String api;
String pos;
String state;
String value;
bool timer = false;
bool manual = false;
constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;
uint32_t previousStateChange;
constexpr int16_t telemetrySendInterval = 2000U;
uint32_t previousDataSend;
EthernetClient client;
Arduino_MQTT_Client mqttClient(client);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);
// constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
//   LED_STATE_ATTR,
//   BLINKING_INTERVAL_ATTR
// };

// // List of client attributes for requesting them (Using to initialize device states)
// constexpr std::array<const char *, 1U> CLIENT_ATTRIBUTES_LIST = {
//   LED_MODE_ATTR
// };

// RPC_Response processSetLedMode(const RPC_Data &data) {
//   Serial.println("Received the set led state RPC method");

//   // Process data
//   int new_mode = data;

//   Serial.print("Mode to change: ");
//   Serial.println(new_mode);

//   if (new_mode != 0 && new_mode != 1) {
//     return RPC_Response("error", "Unknown mode!");
//   }

//   ledMode = new_mode;
//   attributesChanged = true;

//   // Returning current mode
//   return RPC_Response("newMode", (int)ledMode);
// }

// const std::array<RPC_Callback, 1U> callbacks = {
//   RPC_Callback{ "setLedMode", processSetLedMode }
// };

void callback(const char *topic, byte *payload, unsigned int length) {

  Serial.println("On message");
  Serial.print("Topic: ");
  Serial.println(topic);

  String payloadString;
  for (int i = 0; i < length; i++) {
    payloadString += char(payload[i]);
  }

  JsonDocument doc;
  deserializeJson(doc, payloadString);
  String method = doc["method"];
  String api = doc["params"]["api"];
  String value = doc["params"]["value"];
  String pos = doc["params"]["data"]["pos"];
  String state = doc["params"]["data"]["state"];
  Serial.print("Lokasi :");
  Serial.println(api);
  Serial.print("Control Mode : ");
  Serial.println(value);
  Serial.print("Pos : ");
  Serial.println(pos);
  Serial.print("State : ");
  Serial.println(state);
  if (value == "Manual") {
    // Serial.println("Mode Manual");
    manual = true;
    if (pos == "ALL") {
      if (state == "ON") {
        digitalWrite(RelayRA, HIGH);
        digitalWrite(RelaySA, HIGH);
        digitalWrite(RelayTA, HIGH);
        digitalWrite(RelayRB, HIGH);
        digitalWrite(RelaySB, HIGH);
        digitalWrite(RelayTB, HIGH);
      } else {
        digitalWrite(RelayRA, LOW);
        digitalWrite(RelaySA, LOW);
        digitalWrite(RelayTA, LOW);
        digitalWrite(RelayRB, LOW);
        digitalWrite(RelaySB, LOW);
        digitalWrite(RelayTB, LOW);
      }
    }
    if (pos == "RA") {
      if (state == "ON") {
        digitalWrite(RelayRA, HIGH);
        if(digitalRead(RelayRA == 1)){
          tb.sendTelemetryData("RelayRA","1");
        }
      } else {
        digitalWrite(RelayRA, LOW);
        if(digitalRead(RelayRA == 0 )){
          tb.sendTelemetryData("RelayRA","0");
        }
      }
    } else if (pos == "SA") {
      if (state == "ON") {

        digitalWrite(RelaySA, HIGH);
        if(digitalRead(RelaySA == 1)){
          tb.sendTelemetryData("RelaySA","1");
        }
      } else {
        digitalWrite(RelaySA, LOW);
        if(digitalRead(RelaySA == 0)){
          tb.sendTelemetryData("RelaySA","1");
        }
      }
    } else if (pos == "TA") {
      if (state == "ON") {

        digitalWrite(RelayTA, HIGH);
        if(digitalRead(RelayTA == 1)){
          tb.sendTelemetryData("RelayTA","1");
        }
      } else {
        digitalWrite(RelayTA, LOW);
        if(digitalRead(RelayTA == 0)){
          tb.sendTelemetryData("RelayTA","1");
        }
      }
    } else if (pos == "RB") {
      if (state == "ON") {

        digitalWrite(RelayRB, HIGH);
        if(digitalRead(RelayRB == 1)){
          tb.sendTelemetryData("RelayRB","1");
        }
      } else {
        digitalWrite(RelayRB, LOW);
          if(digitalRead(RelayRB == 0)){
          tb.sendTelemetryData("RelayRB","0");
        }
      }
    } else if (pos == "SB") {
      if (state == "ON") {

        digitalWrite(RelaySB, HIGH);
          if(digitalRead(RelaySB == 1)){
          tb.sendTelemetryData("RelaySB","1");
        }
      } else {
        digitalWrite(RelaySB, LOW);
          if(digitalRead(RelaySB == 1)){
          tb.sendTelemetryData("RelaySB","1");
        }
      }
    } else if (pos == "TB") {
      if (state == "ON") {
        digitalWrite(RelayTB, HIGH);
        if(digitalRead(RelayTB == 1)){
          tb.sendTelemetryData("RelayTB","1");
        }
      } else {
        digitalWrite(RelayTB, LOW);
        if(digitalRead(RelayTB == 1)){
          tb.sendTelemetryData("RelayTB","1");
        }
      }
    }else if (pos == "PC") {
      if (state == "ON") {
        statusPC = 1;
        // digitalWrite(RelayTB, HIGH);
        // if(digitalRead(RelayTB == 1)){
        //   tb.sendTelemetryData("RelayTB","1");
        // }
      } else {
        statusPC = 0;
      }
    }
  }
}
void initConnection() {
  Ethernet.init(10);
  // pinMode(SS, OUTPUT);
  // digitalWrite(SS, LOW);
  Serial.println("Connecting");
  Ethernet.begin(mac, ip, dns, gw, subnet);
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.print(".");
    delay(1000);
    if (Ethernet.linkStatus() == LinkON) {
      break;
    }
  }
  Serial.print("Connected (static method)!, IP Add :");
  Serial.println(Ethernet.localIP());
  Serial.print("Gateway : ");
  Serial.println(Ethernet.gatewayIP());

  mqttClient.set_server(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  mqttClient.set_data_callback(callback);
  mqttClient.subscribe("v1/devices/me/rpc/request/+");
  delay(2000);
  tb.sendTelemetryData("ip", Ethernet.localIP());
  tb.sendTelemetryData("mac", mac);
  // tb.sendAttributeData("")
}
void setup() {
  Serial.begin(115200);
  // Serial.println(WiFi.macAddress());
  // Serial1.begin(9600,SERIAL_8N1,18,17);
  // for(int i = 0; i < NUM_PZEMS; i++)
  //   {

  //       pzems[i] = PZEM004Tv30(Serial1, RXD1, TXD1, 0x10 + i);
  //   }
  // serialPZEMR.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, 17, 18);
  pinMode(RelayRA, OUTPUT);
  pinMode(RelaySA, OUTPUT);
  pinMode(RelayTA, OUTPUT);
  pinMode(RelayRB, OUTPUT);
  pinMode(RelaySB, OUTPUT);
  pinMode(RelayTB, OUTPUT);
  pinMode(4, INPUT);

  // delay(1000);
  digitalWrite(RelayRA, LOW);
  // delay(1000);
  digitalWrite(RelaySA, LOW);
  // delay(1000);
  digitalWrite(RelayTA, LOW);
  // delay(1000);
  digitalWrite(RelayRB, LOW);
  // delay(1000);
  digitalWrite(RelaySB, LOW);
  // delay(1000);
  digitalWrite(RelayTB, LOW);
  if (!rtc.begin()) {
    Serial.print("RTC gagal !");
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (rtc.lostPower()) {
    // Serial.println("RTC lost power, let's set the time!");
  }
  initConnection();
}
void loop() {
  int readPhotocell = digitalRead(4);
  Serial.print("readPhotoCell :");
  Serial.println(readPhotocell);
  Serial.print("photoCellOFF :");
  Serial.println(photoCellOFF);
  Serial.print("photoCellON :");
  Serial.println(photoCellON);
  
  if (readPhotocell == 0) {
    photoCellOFF += 1;
    if (photoCellOFF > 2) {

      digitalWrite(RelayRA, LOW);
      digitalWrite(RelaySA, LOW);
      digitalWrite(RelayTA, LOW);
      digitalWrite(RelayRB, LOW);
      digitalWrite(RelaySB, LOW);
      digitalWrite(RelayTB, LOW);
      photoCellOFF = 0;
      photoCellON = 0;
    }
  } else {
    photoCellON += 1;
    if (photoCellON > 2) {
      digitalWrite(RelayRA, HIGH);
      digitalWrite(RelaySA, HIGH);
      digitalWrite(RelayTA, HIGH);
      digitalWrite(RelayRB, HIGH);
      digitalWrite(RelaySB, HIGH);
      digitalWrite(RelayTB, HIGH);
      photoCellON = 0;
      photoCellOFF = 0;
    }
  }
  publishes();
  if (!tb.connected()) {
    initConnection();
    while (!tb.connected()) {
      initConnection();
      DateTime now = rtc.now();
      if (now.hour() >= 18 && now.minute() == 0) {
        digitalWrite(RelayRA, HIGH);
        digitalWrite(RelaySA, HIGH);
        digitalWrite(RelayTA, HIGH);
        digitalWrite(RelayRB, HIGH);
        digitalWrite(RelaySB, HIGH);
        digitalWrite(RelayTB, HIGH);
      }
      if (now.hour() == 6 && now.minute() >= 0) {
        while (now.hour() == 6 && now.minute() >= 0) {
          if (digitalRead(4) == 0) {
            digitalWrite(RelayRA, LOW);
            digitalWrite(RelaySA, LOW);
            digitalWrite(RelayTA, LOW);
            digitalWrite(RelayRB, LOW);
            digitalWrite(RelaySB, LOW);
            digitalWrite(RelayTB, LOW);
            break;
          }
        }
      }

      if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
        String payload = "{";
        payload += "'temp";
        payload += "':25}";
        payload.toCharArray(attributes, 1000);
        mqttClient.subscribe("v1/devices/me/rpc/request/+");
        Serial.println("Sending current GPIO status ...");
        break;
      } else {
        Serial.println("Thingsboard connection failed");
        Serial.println("Retrying in 5 seconds...");
        delay(1000);
      }
    }
  }
  // if (!tb.connected()) {
  //   Serial.print("Connecting to: ");
  //   Serial.print(THINGSBOARD_SERVER);
  //   Serial.print(" with token ");
  //   Serial.println(TOKEN);
  //   if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
  //     Serial.println("Failed to connect");
  //     return;
  //   }
  //   Serial.println("Subscribing for RPC...");
  //   if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
  //     Serial.println("Failed to subscribe for RPC");
  //     return;
  //   }
  //   Serial.println("Subscribe done");

  tb.loop();
  mqttClient.loop();
  delay(1000);
}

void publishes() {

  //   float voltR = pzems[0].voltage();
  // float voltS = pzems[1].voltage();
  // float voltT = pzems[2].voltage();
  // float kwhR = pzems[0].energy();
  // float kwhS = pzems[1].energy();
  // float kwhT = pzems[2].energy();
  // float dayaR = pzems[0].power();
  // float dayaS = pzems[1].power();
  // float dayaT = pzems[2].power();
  // float arusR = pzems[0].current();
  // float arusS = pzems[1].current();
  // float arusT = pzems[2].current();
  // tb.sendTelemetryData("kwhr",kwhR);
  // tb.sendTelemetryData("kwhs",kwhS);
  // tb.sendTelemetryData("kwht",kwhT);
  // tb.sendTelemetryData("voltr",voltR);
  // tb.sendTelemetryData("volts",voltS);
  // tb.sendTelemetryData("voltt",voltT);
  // tb.sendTelemetryData("currentr",arusR);
  // tb.sendTelemetryData("currents",arusS);
  // tb.sendTelemetryData("currentt",arusT);
  int readPhotocell = digitalRead(4);
  // Serial.println(readPhotocell);
  DateTime now = rtc.now();
  sprintf(buffer, "%02d:%02d", now.hour(), now.minute());
  Serial.println(buffer);
  tb.sendTelemetryData("photocell_1", readPhotocell);
  tb.sendTelemetryData("time", buffer);
  tb.sendTelemetryData("kwhr", 140);
  tb.sendTelemetryData("kwhs", 150);
  tb.sendTelemetryData("kwht", 155);
  tb.sendTelemetryData("voltr", 232);
  tb.sendTelemetryData("volts", 235);
  tb.sendTelemetryData("voltt", 239);
  tb.sendTelemetryData("currentr", 15);
  tb.sendTelemetryData("currents", 17);
  tb.sendTelemetryData("currentt", 19);
}
