/*
SUMO
 */

#define WEMOS

#ifdef WEMOS
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
 #include <ArduinoJson.h>
 #endif

#include <Ultrasonic.h>
#include "HW354_Controller.h"

// ======= DEFINES ======
//#define LINE_TRIGGER 1
//#define RADAR 1

#ifdef WEMOS
  // NOTE:
  // Mettere il gnd delle batterie sul PIN accanto a 5V
  // a volte non parte, riprovare

  //definizione dei pin
  #define MOT_LEFT_DIG_1 D6  // must be PWM PINS
  #define MOT_LEFT_DIG_2 D7  // must be PWM PINS

  #define MOT_RIGHT_DIG_1 D4  // must be PWM PINS
  #define MOT_RIGHT_DIG_2 D5  // must be PWM PINS

  #define DIST_FRONTALE_ECHO D3
  #define DIST_FRONTALE_TRIGGER D4

  #define LINE_TRIGGER_PIN  D8
#else
  //definizione dei pin
  #define MOT_A_DIG_1 10  // must be PWM PINS
  #define MOT_A_DIG_2 11  // must be PWM PINS

  #define DIST_FRONTALE_ECHO 2
  #define DIST_FRONTALE_TRIGGER 3

  #define LINE_TRIGGER_PIN  4 
#endif

// ======= 

const char* ssid = "VIOLAFILIPPO";
const char* password = "alicepi1";

ESP8266WebServer server(80);

#ifdef RADAR
Ultrasonic ultrasonic(DIST_FRONTALE_TRIGGER,DIST_FRONTALE_ECHO); // (Trig PIN,Echo PIN)
#endif

HW354_Controller leftMotor("LEFT", MOT_LEFT_DIG_1, MOT_LEFT_DIG_2);
HW354_Controller rightMotor("RIGHT", MOT_RIGHT_DIG_1, MOT_RIGHT_DIG_2);

class Sumo
{
  public:
      bool testMode;
      bool breakMode;
      int steering; //  -100   100
      int speed; // 0  100

  Sumo():testMode(true),steering(0),speed(0),breakMode(false){}

  void tick(){
 
    if (breakMode)
    {
      leftMotor.setSpeed(0);
      rightMotor.setSpeed(0);
      return;
    }
    
    if (testMode) return;
    
    float steeringInvert = -1;

    float left = float(speed)  + (float)steering * steeringInvert*0.6f;
    float right = float(speed)  -  (float)steering * steeringInvert*0.6f;

     // clamp to -100 , 100
    if (left >100) 
    {
        float factor = 100.0f / left;
        left = factor * left;
        right = factor * right;
    }
    if (right >100) 
    {
        float factor = 100.0f / right;
        left = factor * left;
        right = factor * right;
    }

    leftMotor.setSpeed(left);
    rightMotor.setSpeed(right);
  }
};

Sumo sumo;

// ========================================

void sendResponse(DynamicJsonDocument &doc)
{
      String buf;
      serializeJson(doc, buf);
      server.send(200, F("application/json"), buf);
}

void ping() {
   Serial.print(F("ping"));
   server.send ( 200, "text/json", "{\"success\":true}" );
}

bool ReadLineTrigger()
{
  #ifdef LINE_TRIGGER
  return !digitalRead(LINE_TRIGGER_PIN); 
  #else
  return false;
  #endif
}
int ReadRadarCM()
{
  #ifdef RADAR
     int distanza_cm = ultrasonic.read();
   #endif
    int distanza_cm =0;
   return distanza_cm;
}

void dump() {
   DynamicJsonDocument doc(512);
   doc["testMode"] = sumo.testMode;

   int distanza_cm = ReadRadarCM();
   int lineON =  ReadLineTrigger();   // read the input pin
   doc["breakMode"] = sumo.breakMode;
   doc["testMode"] = sumo.testMode;
   doc["dist_cm"] = distanza_cm;
   doc["lineON"] = (lineON == 1 )? "true": "false";
   doc["motor_left"] = leftMotor.speed;
   doc["motor_right"] = rightMotor.speed;
   doc["steering"] = sumo.steering;
   doc["speed"] = sumo.speed;
   sendResponse(doc);
}

void setMode() {
   Serial.print(F("setMode.."));
   String testMode = server.arg("testMode");
   sumo.testMode = testMode=="true";
   Serial.println(testMode);
   server.send ( 200, "text/json", "{\"success\":true}" );
}

void setMotor() {
 //  Serial.print(F("setMotor.."));
   String id = server.arg("id");
   String speed = server.arg("speed");
   if (id =="left")
      leftMotor.setSpeed(speed.toInt());
  else
      rightMotor.setSpeed(speed.toInt());
   server.send ( 200, "text/json", "{\"success\":true}" );
}
void setBreak() {
   Serial.print(F("setBreak.."));
   String breakMode = server.arg("breakMode");
   sumo.breakMode = breakMode=="true";
   server.send ( 200, "text/json", "{\"success\":true}" );
}
void setSumo() {
 //  Serial.print(F("setSumo.."));
   String steering = server.arg("steering");
   String speed = server.arg("speed");
   if (steering !="")
      sumo.steering = steering.toInt();
  if (speed !="")
      sumo.speed = speed.toInt();
   server.send ( 200, "text/json", "{\"success\":true}" );
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the SUMO REST Web Server"));
    });
    server.on(F("/ping"), HTTP_GET, ping);
    server.on(F("/mode"), HTTP_GET, setMode);
    server.on(F("/dump"), HTTP_GET, dump);
    server.on(F("/motor/set"), HTTP_GET, setMotor);
    server.on(F("/motor/break"), HTTP_GET, setBreak);
    server.on(F("/sumo/set"), HTTP_GET, setSumo);
}
  
void setup_WIFI(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}
// ================================================

void setup() {

   Serial.begin(9600); // Pour a bowl of Serial
   Serial.print("Starting.. ");

   setup_WIFI();

   leftMotor.init();
   rightMotor.init();

#ifdef LINE_TRIGGER
   pinMode(LINE_TRIGGER_PIN, INPUT); 
#endif
  // leftMotor.setSpeed(100);

   Serial.print("Done");
}

void loop() {

  server.handleClient();

  // sensore ultrasuono

  //int distanza_cm = ultrasonic.read();
//  int lineON =  ReadLineTrigger();   // read the input pin

  sumo.tick();

/*
  Serial.print("DIST: ");
  Serial.print(distanza_cm);
  Serial.print(" LINE: ");
  Serial.println(lineON);
  */ 
 }
 
