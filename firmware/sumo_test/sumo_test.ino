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

#ifdef WEMOS
  // NOTE:
  // Mettere il gnd delle batterie sul PIN accanto a 5V
  // a volte non parte, riprovare

  //definizione dei pin
  #define MOT_A_DIG_1 D6  // must be PWM PINS
  #define MOT_A_DIG_2 D7  // must be PWM PINS

  #define DIST_FRONTALE_ECHO D3
  #define DIST_FRONTALE_TRIGGER D4

  #define LINE_TRIGGER_PIN  D2
#else
  //definizione dei pin
  #define MOT_A_DIG_1 10  // must be PWM PINS
  #define MOT_A_DIG_2 11  // must be PWM PINS

  #define DIST_FRONTALE_ECHO 2
  #define DIST_FRONTALE_TRIGGER 3

  #define LINE_TRIGGER_PIN  4 
#endif

// ======= 

Ultrasonic ultrasonic(DIST_FRONTALE_TRIGGER,DIST_FRONTALE_ECHO); // (Trig PIN,Echo PIN)

HW354_Controller leftMotor("LEFT", MOT_A_DIG_1, MOT_A_DIG_2);

void setup() {

   Serial.begin(9600); // Pour a bowl of Serial
   Serial.print("Starting.. ");

   leftMotor.init();

   pinMode(LINE_TRIGGER_PIN, INPUT); 

  // leftMotor.setSpeed(100);

   Serial.print("Done");
}

void loop() {

  // sensore ultrasuono

  int distanza_cm = ultrasonic.read();
  int lineON =  digitalRead(LINE_TRIGGER_PIN);   // read the input pin

  Serial.print("DIST: ");
  Serial.print(distanza_cm);
  Serial.print(" LINE: ");
  Serial.println(lineON);
   
 }
 
