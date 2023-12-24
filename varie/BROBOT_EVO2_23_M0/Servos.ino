// BROBOT EVO 2 by JJROBOTS
// SELF BALANCE ARDUINO ROBOT WITH STEPPER MOTORS
// License: GPL v2
// Servo and aux functions

// Default servo definitions
#define SERVO1_AUX_NEUTRO 1500  // Servo neutral position
#define SERVO1_MIN_PULSEWIDTH 700
#define SERVO1_MAX_PULSEWIDTH 2300
#define SERVO2_AUX_NEUTRO 1500  // Servo neutral position
#define SERVO2_MIN_PULSEWIDTH 700
#define SERVO2_MAX_PULSEWIDTH 2300

#define BATT_VOLT_FACTOR 8
int battery;

// Init servo on T4 timer. Output OC4B (Leonardo Pin10)
// We configure the Timer4 for 11 bits PWM (enhacend precision) and 16.3ms period (OK for most servos)
// Resolution: 8us per step (this is OK for servos, around 175 steps for typical servo)
void BROBOT_initServo()
{
  servo1.attach(3);
  servo2.attach(4);
}

void BROBOT_disableServo()
{
  servo1.detach();
  servo2.detach();
}

void BROBOT_moveServo1(int pwm)
{
  pwm = constrain(pwm,SERVO1_MIN_PULSEWIDTH,SERVO1_MAX_PULSEWIDTH);
  servo1.writeMicroseconds(pwm);
}

void BROBOT_moveServo2(int pwm)
{
  pwm = constrain(pwm,SERVO2_MIN_PULSEWIDTH,SERVO2_MAX_PULSEWIDTH);
  servo2.writeMicroseconds(pwm);
}

// output : Battery voltage*10 (aprox) and noise filtered
int BROBOT_readBattery(bool first_time)
{
  if (first_time)
	battery = analogRead(A0)/BATT_VOLT_FACTOR;
  else
    battery = (battery*9 + (analogRead(A0)/BATT_VOLT_FACTOR))/10;
  return battery;
}
