#include "MPU9250.h"

// https://github.com/hideakitai/MPU9250/tree/master
// VCC a +1
// va su a4- a5
// va su SCL/SDA sia UNO che WEMOS
MPU9250 mpu; // You can also use MPU9255 as is

void setup() {
    Serial.begin(9600);
     Serial.println("setup");
    Wire.begin();
    delay(2000);

    mpu.setup(0x68);  // change to your own address
}

void loop() {
    if (mpu.update()) {
        Serial.print(mpu.getYaw()); Serial.print(", ");
        Serial.print(mpu.getPitch()); Serial.print(", ");
        Serial.println(mpu.getRoll());
    }
}