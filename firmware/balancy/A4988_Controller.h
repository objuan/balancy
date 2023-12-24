#include "Arduino.h"

#define MOTOR_DEBUG
#include <AccelStepper.h>

#define MAX_STEPS_BY_SECONDS 600 // provare vari valori per il motore in speed 100
#define numStepMotore 200
/*
A4988_Controller motor driver
MOT_DIG_1   PNW
MOT_DIG_2   PNW
*/
class A4988_Controller
{
  public:
  int speed; // 0 -100
  int MOT_STEP_PIN;
  int MOT_DIR_PIN;
  char name[10];
   AccelStepper *stepper;
  float speedToSteps;

  volatile float stepsPerSeconds;
  volatile float steps;

  A4988_Controller(const  char *_name, int MOT_STEP_PIN,int MOT_DIR_PIN):speed(-1),MOT_STEP_PIN(MOT_STEP_PIN),MOT_DIR_PIN(MOT_DIR_PIN) ,steps(0){
  
    strcpy(name,_name);

    stepper = new AccelStepper(AccelStepper::DRIVER,MOT_STEP_PIN, MOT_DIR_PIN);

    speedToSteps = MAX_STEPS_BY_SECONDS / numStepMotore;

    stepper->setMaxSpeed(MAX_STEPS_BY_SECONDS);
    stepper->setSpeed(0);
  }

  ~A4988_Controller()
  {
    delete stepper;
  }

  void init(){
     //inizializzo i PIN come OUTPUT
  //  pinMode(MOT_STEP_PIN, OUTPUT);
    //pinMode(MOT_DIR_PIN, OUTPUT); 

    //setSpeed(0);
    stepper->setSpeed(0);
  }

  //  -100  100
  void setSpeed(int speed)
  {
    if (speed == this->speed )
      return;

    this->speed=speed;

  #ifdef MOTOR_DEBUG
    Serial.print(name);
    Serial.print(" ");
    Serial.println(speed);
  #endif

   speed = constrain(speed, -100,100);
   speed = map(speed,-100,100,-MAX_STEPS_BY_SECONDS,MAX_STEPS_BY_SECONDS);
   
    stepper->setSpeed(speed);
  }

  void moveTo(long absolute){
      stepper->moveTo(absolute);
  }
  
  ///giri al secondo
  void setSpeedGS(float turnForSeconds){
    int speed = int(turnForSeconds * numStepMotore);
     stepper->setSpeed(speed);
  }

  void tick(float dt)
  {
    steps += stepsPerSeconds * dt;
    stepper->runSpeed();
  }
  
};