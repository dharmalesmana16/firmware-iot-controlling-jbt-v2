#include <Arduino_MQTT_Client.h>
#include <ArduinoJson.h>
#include <ThingsBoard.h>
#include <Ethernet.h>
#include <SPI.h>
#include <EEPROM.h>
#include <PZEM004Tv30.h>
#include <DS3231.h> 
#include <Update.h>
#define RXD1 18
#define TXD1 17
#define NUM_PZEMS 3
PZEM004Tv30 pzems[NUM_PZEMS];

IPAddress eth_address(192, 168, 10, 61);
IPAddress eth_gateway(192, 168, 10, 1);
IPAddress eth_dns(8,8,8,8);
IPAddress eth_subnet(255, 255, 255, 0);
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// constexpr char TOKEN[] = "y5hnevpq530nrukrpu9u"; //cloud
constexpr char TOKEN[] = "AI7ZgmhjWX73Ry2S6Kyu"; //local uturnakame
constexpr char THINGSBOARD_SERVER[] = "43.252.159.216";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
EthernetClient client;
Arduino_MQTT_Client mqttClient(client);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);
char attributes[1000];
char msg[300];

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";
volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;
const int RelayRA = 40;
const int RelaySA = 39; 
const int RelayTA = 38;
const int RelayRB = 37;
const int RelaySB = 36;
const int RelayTB = 35;
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

// For telemetry
constexpr int16_t telemetrySendInterval = 2000U;
uint32_t previousDataSend;


/*
 * Server Index Page
 */


// List of shared attributes for subscribing to their updates
constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR
};

// List of client attributes for requesting them (Using to initialize device states)
constexpr std::array<const char *, 1U> CLIENT_ATTRIBUTES_LIST = {
  LED_MODE_ATTR
};

RPC_Response processSetLedMode(const RPC_Data &data) {
  Serial.println("Received the set led state RPC method");

  // Process data
  int new_mode = data;

  Serial.print("Mode to change: ");
  Serial.println(new_mode);

  if (new_mode != 0 && new_mode != 1) {
    return RPC_Response("error", "Unknown mode!");
  }

  ledMode = new_mode;
  attributesChanged = true;

  // Returning current mode
  return RPC_Response("newMode", (int)ledMode);
}

const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "setLedMode", processSetLedMode }
};

void callback(const char *topic, byte *payload, unsigned int length)
{

    Serial.println("On message");
    Serial.print("Topic: ");
    Serial.println(topic);

    String payloadString;
    for (int i = 0; i < length; i++)
    {
        payloadString += char(payload[i]);
    } 

    JsonDocument doc;
    deserializeJson(doc, payloadString);
    String method = doc["method"];
    String api= doc["params"]["api"];
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
    if(value == "Manual"){
  // Serial.println("Mode Manual");
    manual = true;
    if(pos == "ALL"){
      if(state == "ON"){
      digitalWrite(RelayRA,HIGH);
      digitalWrite(RelaySA,HIGH);
      digitalWrite(RelayTA,HIGH);
      digitalWrite(RelayRB,HIGH);
      digitalWrite(RelaySB,HIGH);
      digitalWrite(RelayTB,HIGH);
      }else{
      digitalWrite(RelayRA,LOW);
      digitalWrite(RelaySA,LOW);
      digitalWrite(RelayTA,LOW);
      digitalWrite(RelayRB,LOW);
      digitalWrite(RelaySB,LOW);
      digitalWrite(RelayTB,LOW);
      }  
    }
    if(pos == "RA"){
      if(state == "ON"){
        digitalWrite(RelayRA,HIGH);
      }else{
        digitalWrite(RelayRA,LOW);
      }   
    }else if (pos == "SA"){
       if(state == "ON"){

        digitalWrite(RelaySA,HIGH);
      }else{
        digitalWrite(RelaySA,LOW);
      }   
    }else if (pos == "TA"){
       if(state == "ON"){

        digitalWrite(RelayTA,HIGH);
      }else{
        digitalWrite(RelayTA,LOW);
      }   
    } else if (pos == "RB"){
       if(state == "ON"){

        digitalWrite(RelayRB,HIGH);
      }else{
        digitalWrite(RelayRB,LOW);
      }   
    }else if (pos == "SB"){
       if(state == "ON"){

        digitalWrite(RelaySB,HIGH);
      }else{
        digitalWrite(RelaySB,LOW);
      }   
    }else if (pos == "TB"){
       if(state == "ON"){

        digitalWrite(RelayTB,HIGH);
      }else{
        digitalWrite(RelayTB,LOW);
      }   
    }         
        
  }
}

void setup() {
  Serial.begin(115200);
    // Serial1.begin(9600,SERIAL_8N1,18,17);
  for(int i = 0; i < NUM_PZEMS; i++)
    {

        pzems[i] = PZEM004Tv30(Serial1, RXD1, TXD1, 0x10 + i);
    }
   	// serialPZEMR.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, 17, 18);
  pinMode(RelayRA,OUTPUT);
  pinMode(RelaySA,OUTPUT);
  pinMode(RelayTA,OUTPUT);
  pinMode(RelayRB,OUTPUT);
  pinMode(RelaySB,OUTPUT);
  pinMode(RelayTB,OUTPUT);  
  delay(1000);
  Ethernet.init(5); // 5 for esp32 10 for Arduino CS PIN
  Serial.println("Connecting");
  Ethernet.begin(mac, eth_address, eth_dns, eth_gateway, eth_subnet);
  while (Ethernet.linkStatus() == LinkOFF)
  {
      Serial.print(".");
      delay(1000);
      if (Ethernet.linkStatus() == LinkON)
      {
          break;
      }
  }
  Serial.print("Connected (static method)!, IP Add :");
  Serial.print(Ethernet.localIP());
  Serial.println("Gateway : ");
  Serial.print(Ethernet.gatewayIP());

  mqttClient.set_server(THINGSBOARD_SERVER,THINGSBOARD_PORT);
  mqttClient.set_callback(callback);
  mqttClient.subscribe("v1/devices/me/rpc/request/+");
  
}
void loop() {
  publishes();
  // float voltR = pzems[0].voltage();
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


  // tb.sendTelemetryData("kwhr",220);
  // tb.sendTelemetryData("kwhs",220);
  // tb.sendTelemetryData("kwht",220);
  // tb.sendTelemetryData("voltr",230);
  // tb.sendTelemetryData("volts",230);
  // tb.sendTelemetryData("voltt",230);
  // tb.sendTelemetryData("currentr",240);
  // tb.sendTelemetryData("currents",240);
  // tb.sendTelemetryData("currentt",240);
  // Serial.print("Volt R : ");
  // Serial.println(voltR);
  // Serial.print("Volt S : ");
  // Serial.println(voltS);
  if(!tb.connected()){
        while (!tb.connected()){
          if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
                {
                String payload = "{";
                payload +="'temp";
                payload += "':25}";
                payload.toCharArray(attributes, 1000);
                mqttClient.subscribe("v1/devices/me/rpc/request/+");  
                Serial.println("Sending current GPIO status ...");
                break;
                }
                else
                {
                    Serial.println("Thingsboard connection failed");
                    Serial.println("Retrying in 5 seconds...");
                    delay(5000);
                }
          }
  }
  if (!tb.connected()) {
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect");
      return;
    }
    Serial.println("Subscribing for RPC...");
    if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }
    Serial.println("Subscribe done");
  }
  tb.loop();
  delay(1000);
}

void publishes(){
    float voltR = pzems[0].voltage();
  float voltS = pzems[1].voltage();
  float voltT = pzems[2].voltage();
  float kwhR = pzems[0].energy();
  float kwhS = pzems[1].energy();
  float kwhT = pzems[2].energy();
  float dayaR = pzems[0].power();
  float dayaS = pzems[1].power();
  float dayaT = pzems[2].power();
  float arusR = pzems[0].current();
  float arusS = pzems[1].current();
  float arusT = pzems[2].current();
  tb.sendTelemetryData("kwhr",kwhR);
  tb.sendTelemetryData("kwhs",kwhS);
  tb.sendTelemetryData("kwht",kwhT);
  tb.sendTelemetryData("voltr",voltR);
  tb.sendTelemetryData("volts",voltS);
  tb.sendTelemetryData("voltt",voltT);
  tb.sendTelemetryData("currentr",arusR);
  tb.sendTelemetryData("currents",arusS);
  tb.sendTelemetryData("currentt",arusT);
}

