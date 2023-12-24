#include <Ultrasonic.h>
#include "HW354_Controller.h"

#define MOT_A_DIG_1 10  // must be PWM PINS
#define MOT_A_DIG_2 11  // must be PWM PINS

int speed = 0;


HW354_Controller leftMotor("LEFT", MOT_A_DIG_1, MOT_A_DIG_2);

void setup()
{
  leftMotor.init();
 // pinMode(MOT_A_DIG_1, OUTPUT); //output perche' definisce lo stato logico del pin IN1 del modulo L298N
  //pinMode(MOT_A_DIG_2, OUTPUT); //output perche' definisce lo stato logico del pin IN2 del modulo L298N

  Serial.begin(9600);
  Serial.println("Enter number for control option:");
  Serial.println("1. FORWARD");
  Serial.println("2. REVERSE");
  Serial.println("3. STOP");
  Serial.println();  

  speed=0;
 // motoreSinistro_HW354(0);
}

/*
 *  power  -100   100
 */
void motoreSinistro_HW354(int potenza)
{
  Serial.print("left: ");
  Serial.println(potenza);

  if (potenza>=0)
  {
    analogWrite(MOT_A_DIG_1,  map(potenza,0,100,0,255));
    digitalWrite(MOT_A_DIG_2, LOW);
  }
  else
  {
    analogWrite(MOT_A_DIG_2,  map(potenza,-100,0,255,0));
    //analogWrite(MOT_A_DIG_2,  0);
 //   digitalWrite(MOT_A_DIG_2,  HIGH);
    digitalWrite(MOT_A_DIG_1, LOW);
   // Serial.print("--: ");
  }
}


void loop()
{
  char user_input;
  
  while(Serial.available())
  {
    user_input = Serial.read();

    Serial.println(user_input);

    if (user_input =='1')
    { 
    
      speed=speed+10;
      //motoreSinistro_HW354(speed);
      leftMotor.setSpeed(speed);
      // Forward();
    }
    else if(user_input =='2')
    {
     // Reverse();
       speed=speed-10;
       leftMotor.setSpeed(speed);
    ///  motoreSinistro_HW354(speed);
    }
    else if(user_input =='3')
    {
    //  Stop();
    }
    else  if(user_input != 0 && user_input != 13)
    {
      Serial.println("Invalid option entered.");
    }
  }
}


