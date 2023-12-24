// BROBOT EVO 2 by JJROBOTS
// SELF BALANCE ARDUINO ROBOT WITH STEPPER MOTORS
// License: GPL v2
// Control functions (PID controls, Steppers control...)

#define MINIMUN_TIMER_PERIOD 32000

// PD controller implementation(Proportional, derivative). DT in seconds
float stabilityPDControl(float DT, float input, float setPoint,  float Kp, float Kd)
{
  float error;
  float output;

  error = setPoint - input;

  // Kd is implemented in two parts
  //    The biggest one using only the input (sensor) part not the SetPoint input-input(t-1).
  //    And the second using the setpoint to make it a bit more agressive   setPoint-setPoint(t-1)
  float Kd_setPoint = constrain((setPoint - setPointOld), -8, 8); // We limit the input part...
  output = Kp * error + (Kd * Kd_setPoint - Kd * (input - PID_errorOld)) / DT;
  //Serial.print(Kd*(error-PID_errorOld));Serial.print("\t");
  //PID_errorOld2 = PID_errorOld;
  PID_errorOld = input;  // error for Kd is only the input component
  setPointOld = setPoint;
  return (output);
}


// PI controller implementation (Proportional, integral). DT in seconds
float speedPIControl(float DT, int16_t input, int16_t setPoint,  float Kp, float Ki)
{
  int16_t error;
  float output;

  error = setPoint - input;
  PID_errorSum += constrain(error, -ITERM_MAX_ERROR, ITERM_MAX_ERROR);
  PID_errorSum = constrain(PID_errorSum, -ITERM_MAX, ITERM_MAX);

  //Serial.println(PID_errorSum);

  output = Kp * error + Ki * PID_errorSum * DT; // DT is in miliseconds...
  return (output);
}


float positionPDControl(long actualPos, long setPointPos, float Kpp, float Kdp, int16_t speedM)
{
  float output;
  float P;

  P = constrain(Kpp * float(setPointPos - actualPos), -115, 115); // Limit command
  output = P + Kdp * float(speedM);
  return (output);
}

// M0 board
// MOTOR1
// Timer2 TC5 interrupt
void TC5_Handler (void) 
{
  TC5->COUNT16.INTFLAG.bit.MC0 = 1; //don't change this, it's part of the timer code
  if (dir_M1 == 0)
    return;
  REG_PORT_OUTSET0 = PORT_PA15; // STEP Motor1
  if (dir_M1 > 0)
    steps1--;
  else
    steps1++;
  //position_M1 += dir_M1;
  delayMicroseconds(1);
  REG_PORT_OUTCLR0 = PORT_PA15; // STEP Motor1
}
// MOTOR2
// TIMER 1 : TC3 interrupt 
void TC3_Handler (void) 
{
  TC3->COUNT16.INTFLAG.bit.MC0 = 1; // Interrupt reset
  if (dir_M2 == 0)
    return;
  REG_PORT_OUTSET0 = PORT_PA21; // STEP Motor2
  if (dir_M2 > 0)
    steps2--;
  else
    steps2++;
  //position_M2 += dir_M2;
  delayMicroseconds(1);
  REG_PORT_OUTCLR0 = PORT_PA21; // STEP Motor2
}

// Motor3
// Timer3 TCC2 interrupt
void TCC2_Handler (void) 
{
  TCC2->INTFLAG.bit.MC0 = 1; 
  if (dir_M2 == 0)
    return;
  REG_PORT_OUTSET0 = PORT_PA07; // STEP Motor3
  if (dir_M2 > 0)
    steps2--;
  else
    steps2++;
  //position_M3 += dir_M3;
  delayMicroseconds(1);
  REG_PORT_OUTCLR0 = PORT_PA07; // STEP Motor3
}


//Configures the TC to generate output events at the sample frequency.
//Configures the TC in Frequency Generation mode, with an event output once
//each time the audio sample frequency period expires.
void timersConfigure()
{
 // First we need to enable and configure the Generic Clock register 
 // Enable GCLK for TC4, TC5, TCC2 and TC3 (timer counter input clock) GCLK_CLKCTRL_ID(GCM_TC4_TC5)
 GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC2_TC3));
 while (GCLK->STATUS.bit.SYNCBUSY);
 GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
 while (GCLK->STATUS.bit.SYNCBUSY);
 
 // Configure Timer1
 TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
 while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
 while (TC3->COUNT16.CTRLA.bit.SWRST);

 // Set Timer counter Mode to 16 bits
 TC3->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
 // Set TC5 mode as match frequency
 TC3->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
 //set prescaler and enable TC5
 TC3->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV16 | TC_CTRLA_ENABLE;  // preescaler 16 48Mhz=>3Mhz
 //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
 TC3->COUNT16.CC[0].reg = (uint16_t) MINIMUN_TIMER_PERIOD;
 
 while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
 
 // Configure interrupt request
 NVIC_DisableIRQ(TC3_IRQn);
 NVIC_ClearPendingIRQ(TC3_IRQn);
 NVIC_SetPriority(TC3_IRQn, 0);
 NVIC_EnableIRQ(TC3_IRQn);

 // Enable interrupt request
 TC3->COUNT16.INTENSET.bit.MC0 = 1;
 while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); //wait until syncing

 // Configure Timer2 on TC5
 TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
 while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
 while (TC5->COUNT16.CTRLA.bit.SWRST);

 // Set Timer counter Mode to 16 bits
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
 // Set TC5 mode as match frequency
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
 //set prescaler and enable TC5
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV16 | TC_CTRLA_ENABLE;  // preescaler 16 48Mhz=>3Mhz
 //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
 TC5->COUNT16.CC[0].reg = (uint16_t) MINIMUN_TIMER_PERIOD;
 
 while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
 
 // Configure interrupt request
 NVIC_DisableIRQ(TC5_IRQn);
 NVIC_ClearPendingIRQ(TC5_IRQn);
 NVIC_SetPriority(TC5_IRQn, 0);
 NVIC_EnableIRQ(TC5_IRQn);

 // Enable interrupt request
 TC5->COUNT16.INTENSET.bit.MC0 = 1;
 while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); //wait until syncing

 // Configure Timer3 on TCC2
 TCC2->CTRLA.reg = TC_CTRLA_SWRST;
 //while (TCCC2->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
 //while (TCCC2->COUNT16.CTRLA.bit.SWRST);
 while (TCC2->SYNCBUSY.bit.ENABLE == 1); // wait for sync
 while (TCC2->CTRLA.bit.SWRST);
 
 // Set Timer counter Mode to 16 bits
 //TCC2->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
 // Set TCC2 mode as match frequency
 //TCC2->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
 TCC2->WAVE.reg |= TCC_WAVE_WAVEGEN_MFRQ;
 //set prescaler and enable TCC2
 TCC2->CTRLA.reg |= TCC_CTRLA_PRESCALER_DIV16 | TCC_CTRLA_ENABLE;  // preescaler 16 48Mhz=>3Mhz
 //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
 TCC2->CC[0].reg = (uint16_t)MINIMUN_TIMER_PERIOD;
 TCC2->CTRLBCLR.reg |= TCC_CTRLBCLR_LUPD;   // Enable doble buffering
 
 //while (TCC2->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
 while (TCC2->SYNCBUSY.bit.ENABLE == 1); // wait for sync 
 
 // Configure interrupt request
 NVIC_DisableIRQ(TCC2_IRQn);
 NVIC_ClearPendingIRQ(TCC2_IRQn);
 NVIC_SetPriority(TCC2_IRQn, 0);
 NVIC_EnableIRQ(TCC2_IRQn);

 // Enable interrupt request
 //TCC2->COUNT16.INTENSET.bit.MC0 = 1;
 TCC2->INTENSET.reg = 0;                 // disable all interrupts
 //TCC2->INTENSET.bit.OVF = 1;          // enable overfollow
 TCC2->INTENSET.bit.MC0 = 1;          // enable compare match to CC0
 //while (TCC2->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); //wait until syncing
 while (TCC2->SYNCBUSY.bit.ENABLE == 1); // wait for sync 
} 

// This function enables Timers TC3 and TC5 and waits for it to be ready
void timersStart()
{
  TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); //wait until snyc'd
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); //wait until snyc'd
  TCC2->CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (TCC2->STATUS.reg & TC_STATUS_SYNCBUSY); //wait until snyc'd
}

//Reset timers TC3 and TC5 
void timersReset()
{
  TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
  while (TC3->COUNT16.CTRLA.bit.SWRST);
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
  while (TC5->COUNT16.CTRLA.bit.SWRST);
}

// Disable timers TC3 and TC5
void timersDisable()
{
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
}


// M0 board

//// TIMER 1 : STEPPER MOTOR1 SPEED CONTROL
//ISR(TIMER1_COMPA_vect)
//{
//  if (dir_M1 == 0) // If we are not moving we dont generate a pulse
//    return;
//  // We generate 1us STEP pulse
//  SET(PORTE, 6); // STEP MOTOR 1
//  //delay_1us();
//  if (dir_M1 > 0)
//    steps1--;
//  else
//    steps1++;
//  CLR(PORTE, 6);
//}
//// TIMER 3 : STEPPER MOTOR2 SPEED CONTROL
//ISR(TIMER3_COMPA_vect)
//{
//  if (dir_M2 == 0) // If we are not moving we dont generate a pulse
//    return;
//  // We generate 1us STEP pulse
//  SET(PORTD, 6); // STEP MOTOR 2
//  //delay_1us();
//  if (dir_M2 > 0)
//    steps2--;
//  else
//    steps2++;
//  CLR(PORTD, 6);
//}


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

#if MICROSTEPPING==16
  speed = speed_M1 * 50; // Adjust factor from control output speed to real motor speed in steps/second
#else
  speed = speed_M1 * 25; // 1/8 Microstepping
#endif

  if (speed == 0)
  {
    timer_period = ZERO_SPEED;
    dir_M1 = 0;
  }
  else if (speed > 0)
  {
    timer_period = 3000000 / speed; // 2Mhz timer
    dir_M1 = 1;
    //SET(PORTB, 4); // DIR Motor 1 (Forward)
    REG_PORT_OUTSET0 = PORT_PA20;
  }
  else
  {
    timer_period = 3000000 / -speed;
    dir_M1 = -1;
    //CLR(PORTB, 4); // Dir Motor 1
    REG_PORT_OUTCLR0 = PORT_PA20;
  }
  if (timer_period > 65535)   // Check for minimun speed (maximun period without overflow)
    timer_period = ZERO_SPEED;

  // Change timer
  TC5->COUNT16.CC[0].reg = (uint16_t) timer_period;
  while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
  
  // Check  if we need to reset the timer...
  if (TC5->COUNT16.COUNT.reg > (uint16_t)timer_period){
    TC5->COUNT16.COUNT.reg = (uint16_t)timer_period-4;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
  } 
    
}

// Set speed of Stepper Motor2 (outputM3)
// tspeed could be positive or negative (reverse)
void setMotorSpeedM2(int16_t tspeed)
{
  long timer_period;
  int16_t speed;

  // Limit max speed?

  // WE LIMIT MAX ACCELERATION of the motors
  if ((speed_M2 - tspeed) > MAX_ACCEL)
    speed_M2 -= MAX_ACCEL;
  else if ((speed_M2 - tspeed) < -MAX_ACCEL)
    speed_M2 += MAX_ACCEL;
  else
    speed_M2 = tspeed;

#if MICROSTEPPING==16
  speed = speed_M2 * 50; // Adjust factor from control output speed to real motor speed in steps/second
#else
  speed = speed_M2 * 25; // 1/8 Microstepping
#endif

  if (speed == 0)
  {
    timer_period = ZERO_SPEED;
    dir_M2 = 0;
  }
  else if (speed > 0)
  {
    timer_period = 3000000 / speed; // 2Mhz timer
    dir_M2 = 1;
    //CLR(PORTC, 6);   // Dir Motor2 (Forward)
    REG_PORT_OUTCLR0 = PORT_PA06; // Dir Motor2 (Forward)
  }
  else
  {
    timer_period = 3000000 / -speed;
    dir_M2 = -1;
    //SET(PORTC, 6);  // DIR Motor 2
    REG_PORT_OUTSET0 = PORT_PA06;
  }
  if (timer_period > 65535)   // Check for minimun speed (maximun period without overflow)
    timer_period = ZERO_SPEED;

  // Change timer
  TC3->COUNT16.CC[0].reg = (uint16_t) timer_period;
  while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
 
  // Check  if we need to reset the timer...
   if (TC3->COUNT16.COUNT.reg > (uint16_t)timer_period){
    TC3->COUNT16.COUNT.reg = (uint16_t)timer_period-4;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // wait for sync...
  }

}

