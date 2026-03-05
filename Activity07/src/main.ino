#include <Servo.h>
#include <Arduino.h>

Servo myServo;

void setup() {
  Serial.begin(9600);
  myServo.attach(9);
  // Initial position
  myServo.write(90);
}

void loop() {
  if (Serial.available() > 0) {
    // Read the incoming integer
    int angle = Serial.parseInt();
    
    // Validate range and update servo
    if (angle >= 0 && angle <= 180) {
      myServo.write(angle);
    }
    
    // Clear buffer
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}