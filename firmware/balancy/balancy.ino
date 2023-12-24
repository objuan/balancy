/*
SUMO
 */

#define WEMOS
//#define DEBUG 10

#ifdef WEMOS
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
 #include <ArduinoJson.h>
#else
#include <ArduinoJson.h>
 #endif

//
#include "A4988_Controller.h"
#include "MPU9250.h"
#include "Control.h"

// ======= DEFINES ======

#ifdef WEMOS
  // NOTE:
  // Mettere il gnd delle batterie sul PIN accanto a 5V
  // a volte non parte, riprovare

  //definizione dei pin
  #define MOT_LEFT_STEP_PIN D6  // must be PWM PINS
  #define MOT_LEFT_DIR_PIN D7  // must be PWM PINS

  #define MOT_RIGHT_STEP_PIN D4  // must be PWM PINS
  #define MOT_RIGHT_DIR_PIN D5  // must be PWM PINS

#else
  //definizione dei pin
   #define MOT_LEFT_STEP_PIN 3  // must be PWM PINS
  #define MOT_LEFT_DIR_PIN 2  // must be PWM PINS

  #define MOT_RIGHT_STEP_PIN 4  // must be PWM PINS
  #define MOT_RIGHT_DIR_PIN 5  // must be PWM PINS

#endif

// ======================================

// NORMAL MODE PARAMETERS (MAXIMUN SETTINGS)
#define MAX_THROTTLE 550
#define MAX_STEERING 140
#define MAX_TARGET_ANGLE 14

// PRO MODE = MORE AGGRESSIVE (MAXIMUN SETTINGS)
#define MAX_THROTTLE_PRO 780   // Max recommended value: 860
#define MAX_STEERING_PRO 260   // Max recommended value: 280
#define MAX_TARGET_ANGLE_PRO 24   // Max recommended value: 32

// Default control terms for EVO 2
#define KP 0.32       
#define KD 0.050     
#define KP_THROTTLE 0.080 
#define KI_THROTTLE 0.1 
#define KP_POSITION 0.06  
#define KD_POSITION 0.45  

// Control gains for raiseup (the raiseup movement requiere special control parameters)
#define KP_RAISEUP 0.1   
#define KD_RAISEUP 0.16   
#define KP_THROTTLE_RAISEUP 0   // No speed control on raiseup
#define KI_THROTTLE_RAISEUP 0.0

#define ANGLE_OFFSET 0.0  // Offset angle for balance (to compensate robot own weight distribution)
#define MAX_ACCEL 14 //14     // Maximun motor acceleration (MAX RECOMMENDED VALUE: 20) (default:14)

#define MAX_CONTROL_OUTPUT 500 // come la max speed
// ======= 

const char* ssid = "VIOLAFILIPPO";
const char* password = "alicepi1";

#ifdef WEMOS
//#include "WIFI_Controller.h"
#include "NET_Controller.h"
#endif

A4988_Controller leftMotor("LEFT", MOT_LEFT_STEP_PIN, MOT_LEFT_DIR_PIN);
A4988_Controller rightMotor("RIGHT", MOT_RIGHT_STEP_PIN, MOT_RIGHT_DIR_PIN);
//AccelStepper leftMotor(AccelStepper::DRIVER,MOT_LEFT_STEP_PIN, MOT_LEFT_DIR_PIN); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

// https://github.com/hideakitai/MPU9250/tree/master
// VCC a +1
// va su a4- a5
// va su SCL/SDA sia UNO che WEMOS
MPU9250 mpu; // You can also use MPU9255 as is

// ===================================

// Default control values from constant definitions
float Kp = KP;
float Kd = KD;
float Kp_thr = KP_THROTTLE;
float Ki_thr = KI_THROTTLE;
float Kp_user = KP;
float Kd_user = KD;
float Kp_thr_user = KP_THROTTLE;
float Ki_thr_user = KI_THROTTLE;
float Kp_position = KP_POSITION;
float Kd_position = KD_POSITION;

long timer_old;
long timer_net_old;
long timer_value;
float debugVariable;
float dt;

uint8_t cascade_control_loop_counter = 0;
uint8_t loop_counter;       // To generate a medium loop 40Hz
uint8_t slow_loop_counter;  // slow loop 2Hz

// Angle of the robot (used for stability control)
float angle_adjusted;
float angle_adjusted_Old;
float angle_adjusted_filtered=0.0;

//boolean positionControlMode = false;
//uint8_t mode;  // mode = 0 Normal mode, mode = 1 Pro mode (More agressive)

// position control
//volatile int32_t steps1;
//volatile int32_t steps2;
int32_t target_steps1;
int32_t target_steps2;
int16_t motor1_control;
int16_t motor2_control;

int16_t speed_M1, speed_M2;        // Actual speed of motors
int16_t actual_robot_speed;        // overall robot speed (measured from steppers speed)
int16_t actual_robot_speed_Old;
float estimated_speed_filtered;    // Estimated robot speed

float target_angle;
int16_t throttle;
float steering;
float max_throttle = MAX_THROTTLE;
float max_steering = MAX_STEERING;
float max_target_angle = MAX_TARGET_ANGLE;
float control_output;
float angle_offset = ANGLE_OFFSET;

PID_Controller pidController;
// ========================================

//WIFI_Controller net;
NET_Server net;
// COMMANDS




// output
NETVariable<int> var_sensor_angle= net.addVar<int>("MPU.sensor_angle",0);
NETVariable<int> var_angle_adjusted= net.addVar<int>("MPU.angle_adjusted",0);
//input
//uint8_t mode;  // mode = 0 Normal mode, mode = 1 Pro mode (More agressive), 2 only motors

#define WORK_NORMAL 0
#define WORK_PRO 1
#define WORK_MOTOR_TEST 2
NETVariable<int> &var_mode= net.addVar<int>("mode.workMode",WORK_MOTOR_TEST);

NETVariable<bool> &var_moveStepMode= net.addVar<bool>("mode.moveStepMode",false);
NETVariable<bool> &var_positionControlMode= net.addVar<bool>("mode.positionControlMode",false);
NETVariable<int> &var_speed= net.addVar<int>("driver.speed",0);
NETVariable<int> &var_steering= net.addVar<int>("driver.steering",0);
NETVariable<int> &var_left=net.addVar<int>("motors.left",0);
NETVariable<int> &var_right=net.addVar<int>("motors.right",0);



void resetMPU(){
  Serial.println("resetMPU,Calibration MODE  ");
  angle_offset = angle_adjusted_filtered;
  net.sendOK();
}

void sleepCmd(){
  Serial.println("sleepCmd");
    pidController.PID_errorSum  = 0;
    timer_old = millis();
    timer_net_old = timer_old;
    setMotorSpeedM1(0);
    setMotorSpeedM2(0);
    //digitalWrite(4, HIGH);  // Disable motors
    //OSC_MsgRead();
  net.sendOK();
}
void readControlParameters(){
  Serial.println("readControlParameters");
   //TODO
  net.sendOK();
}
void setupCommands()
{
  net.addListener("resetMPU", resetMPU);
  net.addListener("sleep", sleepCmd);
  net.addListener("readControlParameters", readControlParameters);
}


// Set speed of Stepper Motor1
// tspeed could be positive or negative (reverse)
void setMotorSpeedM1(int16_t tspeed)
{
  long timer_period;
  int16_t speed;

  // Limit max speed?

  // WE LIMIT MAX ACCELERATION of the motors
  if ((speed_M1 - tspeed) > MAX_ACCEL)
    speed_M1 -= MAX_ACCEL;
  else if ((speed_M1 - tspeed) < -MAX_ACCEL)
    speed_M1 += MAX_ACCEL;
  else
    speed_M1 = tspeed;
 // invertito
  leftMotor.setSpeed(map(-speed_M1, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT,  -100,100));
}
// Set speed of Stepper Motor1
// tspeed could be positive or negative (reverse)
void setMotorSpeedM2(int16_t tspeed)
{
  long timer_period;
  int16_t speed;

  // Limit max speed?

  // WE LIMIT MAX ACCELERATION of the motors
  if ((speed_M2- tspeed) > MAX_ACCEL)
    speed_M2 -= MAX_ACCEL;
  else if ((speed_M2 - tspeed) < -MAX_ACCEL)
    speed_M2 += MAX_ACCEL;
  else
    speed_M2 = tspeed;

  // invertito
  rightMotor.setSpeed(map(-speed_M2, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT,  -100,100));
}
// ================================================

void setup() {

   Serial.begin(9600); // Pour a bowl of Serial
   Serial.println("Starting.. ");

#ifdef WEMOS
   net.setup();

    net.setConstant("name","Balacy");
    setupCommands();

    net.start();

#endif

    Serial.println("MPU starting.. ");
    Wire.begin();
    delay(2000);

    mpu.setup(0x68);  // change to your own address

    if (mpu.update())
    {
      net.setConstant("MPU.enable",true);
      Serial.println("MPU DONE ");
    }
    else{
      net.setConstant("MPU.enable",false);
      Serial.println("MPU FAILED ");
    }

   leftMotor.init();
   rightMotor.init();

   Serial.println("Done");

  // leftMotor.setSpeedGS(0.5f);
   leftMotor.setSpeed(0);
   rightMotor.setSpeed(0);

  timer_old = micros();
  timer_net_old = timer_old;

}
long lastPos = 0;

void loop() 
{
  /*
    leftMotor.tick(0.1);
    timer_value = micros();
    dt = (timer_value - timer_old) * 0.000001; // dt in seconds
    //timer_old = timer_value;

    if (dt > 1)
    {
      timer_old = timer_value;
      long    pos = leftMotor.stepper->currentPosition();  
      long delta =  pos - lastPos;
      lastPos = pos;
      Serial.print(dt);
      Serial.print(" ");
      Serial.println(delta);
    }
    return;
    */
    // ===================

    timer_value = micros();
    dt = (timer_value - timer_old) * 0.000001; // dt in seconds
    timer_old = timer_value;

  #ifdef WEMOS
    if (timer_value - timer_net_old > 1000*10 ) // 10 ms
    {
        timer_net_old = timer_value;
       // server.handleClient();
       net.tick(dt);
    }
  #endif

  uint8_t mode = var_mode.get();

  if (mode  != WORK_MOTOR_TEST &&  mpu.update()) 
  {
      if (var_moveStepMode.get())
      {
        //SerialUSB.print("M ");
        //SerialUSB.print(OSCmove_speed);
        //SerialUSB.print(" ");
        //SerialUSB.print(OSCmove_steps1);
        //SerialUSB.print(",");
        //SerialUSB.println(OSCmove_steps2);
        var_positionControlMode.set(true);
       // OSCmove_mode = false;
      //  target_steps1 = steps1 + OSCmove_steps1;
       // target_steps2 = steps2 + OSCmove_steps2;
      }
      else
      {
        var_positionControlMode.set(false);

         throttle = map(var_speed.get(),-100,100,-max_throttle,max_throttle) ;//(var_speed - 0.5) * max_throttle;
        // We add some exponential on steering to smooth the center band
         steering = map(var_steering.get(), -100,100 , -max_steering,max_steering);
    /*
        throttle = (OSCfader[0] - 0.5) * max_throttle;
        // We add some exponential on steering to smooth the center band
        steering = OSCfader[1] - 0.5;
        if (steering > 0)
          steering = (steering * steering + 0.5 * steering) * max_steering;
        else
          steering = (-steering * steering + 0.5 * steering) * max_steering;
          */
      }

      if (mode == WORK_PRO)
      {
        // Change to PRO mode
        max_throttle = MAX_THROTTLE_PRO;
        max_steering = MAX_STEERING_PRO;
        max_target_angle = MAX_TARGET_ANGLE_PRO;
      }
      else
      {
        // Change to NORMAL mode
        max_throttle = MAX_THROTTLE;
        max_steering = MAX_STEERING;
        max_target_angle = MAX_TARGET_ANGLE;
      }

    
   // timer_value = micros();

    //Serial.print("mode ");
    //Serial.println(mode);

  #if DEBUG==11
    Serial.print("throttle");
      Serial.print(throttle);
      Serial.print("<->");
      Serial.println(steering);
  #endif


    //Serial.print(mpu.getYaw()); Serial.print(", ");
    //Serial.print(mpu.getPitch()); Serial.print(", ");
          //Serial.println(mpu.getRoll());

    loop_counter++;
    slow_loop_counter++;
  //  dt = (timer_value - timer_old) * 0.000001; // dt in seconds
    //timer_old = timer_value;

    angle_adjusted_Old = angle_adjusted;
    float MPU_sensor_angle = mpu.getRoll(); //TODO
    angle_adjusted = MPU_sensor_angle + angle_offset;
    if ((MPU_sensor_angle>-15)&&(MPU_sensor_angle<15)) // limite
      angle_adjusted_filtered = angle_adjusted_filtered*0.99 + MPU_sensor_angle*0.01;

    // UI
    var_sensor_angle.set( MPU_sensor_angle);
    var_angle_adjusted.set(angle_adjusted);

  #if DEBUG==10
            Serial.print(mpu.getYaw());
            Serial.print(" ");
            Serial.print(mpu.getPitch());
            Serial.print(" ");
            Serial.print(mpu.getRoll());
            Serial.println();
    #endif

    #if DEBUG==1
            Serial.print(int(dt*1000));
            Serial.print(" ");
            Serial.print(angle_adjusted);
            Serial.print(",");
            Serial.print(angle_adjusted_filtered);
            Serial.println();
    #endif

    //int speed_M1 = leftMotor.speed ;
    //int speed_M2 = rightMotor.speed ;

        // We calculate the estimated robot speed:
    // Estimated_Speed = angular_velocity_of_stepper_motors(combined) - angular_velocity_of_robot(angle measured by IMU)
    actual_robot_speed = (speed_M1 + speed_M2) / 2; // Positive: forward  

    int16_t angular_velocity = (angle_adjusted - angle_adjusted_Old) * 25.0; // 25 is an empirical extracted factor to adjust for real units
    int16_t estimated_speed = -actual_robot_speed + angular_velocity;
    estimated_speed_filtered = estimated_speed_filtered * 0.9 + (float)estimated_speed * 0.1; // low pass filter on estimated speed

    #if DEBUG==2
          Serial.print(angle_adjusted);
          Serial.print(" ");
          Serial.println(estimated_speed_filtered);
    #endif

    if (var_positionControlMode.get())
    {
          // POSITION CONTROL. INPUT: Target steps for each motor. Output: motors speed
          motor1_control = pidController.positionPDControl(leftMotor.steps, target_steps1, Kp_position, Kd_position, speed_M1);
          motor2_control = pidController.positionPDControl(rightMotor.steps, target_steps2, Kp_position, Kd_position, speed_M2);

          // Convert from motor position control to throttle / steering commands
          throttle = (motor1_control + motor2_control) / 2;
          throttle = constrain(throttle, -190, 190);
          steering = motor2_control - motor1_control;
          steering = constrain(steering, -50, 50);
    }

    // ROBOT SPEED CONTROL: This is a PI controller.
    //    input:user throttle(robot speed), variable: estimated robot speed, output: target robot angle to get the desired speed
    target_angle = pidController.speedPIControl(dt, estimated_speed_filtered, throttle, Kp_thr, Ki_thr);
    target_angle = constrain(target_angle, -max_target_angle, max_target_angle); // limited output

    #if DEBUG==3
        Serial.print(angle_adjusted);
        Serial.print(" ");
        Serial.print(estimated_speed_filtered);
        Serial.print(" ");
        Serial.println(target_angle);
    #endif

    // Stability control (100Hz loop): This is a PD controller.
    //    input: robot target angle(from SPEED CONTROL), variable: robot angle, output: Motor speed
    //    We integrate the output (sumatory), so the output is really the motor acceleration, not motor speed.
    control_output += pidController.stabilityPDControl(dt, angle_adjusted, target_angle, Kp, Kd);
    control_output = constrain(control_output, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT); // Limit max output from control

    // The steering part from the user is injected directly to the output
    int motor1 = control_output + steering;
    int motor2 = control_output - steering;

    // Limit max speed (control output)
    motor1 = constrain(motor1, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT);
    motor2 = constrain(motor2, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT);

    int angle_ready=0;
    if (false)//OSCpush[0])     // If we press the SERVO button we start to move
        angle_ready = 82;
    else
        angle_ready = 74;  // Default angle

    if ((angle_adjusted < angle_ready) && (angle_adjusted > -angle_ready)) // Is robot ready (upright?)
    {
        // NORMAL MODE
        //digitalWrite(11, LOW);  // Motors enable
        // NOW we send the commands to the motors
        setMotorSpeedM1(motor1);
        setMotorSpeedM2(motor2);
       // leftMotor.setSpeed(motor1);
       // rightMotor.setSpeed(motor2);
    }
    else   // Robot not ready (flat), angle > angle_ready => ROBOT OFF
    {
      //  digitalWrite(11, HIGH);  // Disable motors
       // leftMotor.setSpeed(0);
      //  rightMotor.setSpeed(0);
        setMotorSpeedM1(0);
        setMotorSpeedM2(0);
        pidController.PID_errorSum = 0;  // Reset PID I term
        Kp = KP_RAISEUP;   // CONTROL GAINS FOR RAISE UP
        Kd = KD_RAISEUP;
        Kp_thr = KP_THROTTLE_RAISEUP;
        Ki_thr = KI_THROTTLE_RAISEUP;
        // RESET steps
        leftMotor.steps = 0;
        rightMotor.steps = 0;
        var_positionControlMode.set(false);
      //  OSCmove_mode = false;
        throttle = 0;
        steering = 0;
    }

    // Push1 Move servo arm
    /*
      if (false)//OSCpush[0])  // Move arm
      {
        if (angle_adjusted > -40)
          BROBOT_moveServo1(SERVO_MIN_PULSEWIDTH);
        else
          BROBOT_moveServo1(SERVO_MAX_PULSEWIDTH);
      }
      else
        BROBOT_moveServo1(SERVO_AUX_NEUTRO);

      // Servo2
      BROBOT_moveServo2(SERVO2_NEUTRO + (OSCfader[2] - 0.5) * SERVO2_RANGE);
    */
    // Normal condition?
    if ((angle_adjusted < 56) && (angle_adjusted > -56))
    {
        Kp = Kp_user;            // Default user control gains
        Kd = Kd_user;
        Kp_thr = Kp_thr_user;
        Ki_thr = Ki_thr_user;
    }
    else    // We are in the raise up procedure => we use special control parameters
    {
        Kp = KP_RAISEUP;         // CONTROL GAINS FOR RAISE UP
        Kd = KD_RAISEUP;
        Kp_thr = KP_THROTTLE_RAISEUP;
        Ki_thr = KI_THROTTLE_RAISEUP;
    }
  }
  else
  {
    // MOTOR TEST MODE
     int left = var_left.get();
     int right = var_right.get();

    setMotorSpeedM1(left);
    setMotorSpeedM2(right);

    // leftMotor.setSpeed(left);
     //rightMotor.setSpeed(right);
  }
  
  leftMotor.tick(dt);
  rightMotor.tick(dt);


  //Serial.println(dt * 1000);
}
 
