#include<Servo.h>

#define sleep delay(1000)

Servo myservo;
int32_t pos = 1;

void setup()
{
  pinMode(2,INPUT_PULLUP);
  myservo.attach(3);
  myservo.write(90);
}

void loop()
{
  
  if(digitalRead(2) == LOW){
      if (pos == 1){
        myservo.write(0);
        sleep;
        myservo.write(90);
        pos = 2;
        // Serial.println(pos, DEC);   
      }
      else if (pos == 2){
        myservo.write(180);
        sleep;
        myservo.write(90);
        pos = 1;  
      }
  }

}
