#define MOTOR_DEBUG

/*
HW354 motor driver
MOT_DIG_1   PNW
MOT_DIG_2   PNW
*/
class HW354_Controller
{
  public:
  int speed; // 0 -100
  int MOT_DIG_1;
  int MOT_DIG_2;
  char name[10];

  HW354_Controller(const  char *_name, int MOT_DIG_1,int MOT_DIG_2):speed(-1),MOT_DIG_1(MOT_DIG_1),MOT_DIG_2(MOT_DIG_2) {
  
    strcpy(name,_name);
  }

  void init(){
     pinMode(MOT_DIG_1, OUTPUT); //output perche' definisce lo stato logico del pin IN1 del modulo L298N
    pinMode(MOT_DIG_2, OUTPUT); //output perche' definisce lo stato logico del pin IN2 del modulo L298N

    setSpeed(0);
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

    if (speed>=0)
    {
      analogWrite(MOT_DIG_1,  map(speed,0,100,0,255));
      digitalWrite(MOT_DIG_2, LOW);
    }
    else
    {
      analogWrite(MOT_DIG_2,  map(speed,-100,0,255,0));
      //analogWrite(MOT_A_DIG_2,  0);
  //   digitalWrite(MOT_A_DIG_2,  HIGH);
      digitalWrite(MOT_DIG_1, LOW);
    // Serial.print("--: ");
    }
  }

};


