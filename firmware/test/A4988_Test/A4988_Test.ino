//Definizione dei PIN
const int pinDir = D7;
const int pinStep = D6;

//const int pinDir = D5;
//const int pinStep = D4;

const int numStepMotore = 200; //E' il numero di step per rotazione del motore (potrebbe differire in base al modello)
const long velocita = 1000; //Si tratta di microsecondi tra un impulso e l'altro sul pin STEP

// corrente troppo altra fa vibrare tutto

#define MAX_STEPS_BY_SECONDS 650 // provare vari valori per il motore in speed 100

#include <AccelStepper.h>

 AccelStepper stepper(AccelStepper::DRIVER,pinStep, pinDir);

long timer_old;
long timer_value;
long lastPos;
float dt;

void setup() {
  //inizializzo i PIN come OUTPUT
   Serial.begin(9600);
//  pinMode(pinStep, OUTPUT);
  //pinMode(pinDir, OUTPUT);

    stepper.setMaxSpeed(MAX_STEPS_BY_SECONDS);
    stepper.setAcceleration(10);
    stepper.setSpeed(700); // steps per secondo
    timer_old=0;
    lastPos=0;

     stepper.moveTo(0);
}



void loop() {
//Serial.println("loop");

/*
  // Check if the motor has reached its target position
  if (stepper.distanceToGo() == 0) {
    // If the motor has reached its target position, choose a new random target
    int newPosition = random(-1000, 1000);
    stepper.moveTo(newPosition);
  }

  // Step the motor
  stepper.run();
  return;
  // ===================
*/
  stepper.runSpeed();

    timer_value = micros();
    dt = (timer_value - timer_old) * 0.000001; // dt in seconds
    //timer_old = timer_value;

    if (dt > 1)
    {
      timer_old = timer_value;
      long    pos = stepper.currentPosition();  
      long delta =  pos - lastPos;
      lastPos = pos;
      Serial.print(dt);
      Serial.print(" ");
      Serial.println(delta);
    }
    
    return;

  //definiamo la direzione del motore
  digitalWrite(pinDir, HIGH);

  //esegue un giro completo in un senso
  for (int x = 0; x < numStepMotore; x++) {
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(velocita);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(velocita);
  }
Serial.println("inv");
  //aspetto 2 secondi
  delay(2000);

Serial.println("inv1");
  //cambio la direzione di marcia
  digitalWrite(pinDir, LOW);

  //rieseguo un altro giro completo nel senso opposto
  for (int x = 0; x < numStepMotore; x++) {
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(velocita);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(velocita);
  }

  //aspetto altri 2 secondi
  delay(2000);
  Serial.println("inv1");
}