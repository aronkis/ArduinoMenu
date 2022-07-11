#include<LiquidCrystal_I2C.h>
#include "RTClib.h"
#include<Servo.h>

#define sleep delay(1000)
#define DEBUG Serial.print("\nIN: "); Serial.print(__func__); Serial.print(" ON LINE: "); Serial.print(__LINE__)
#define MENUSIZE 4
#define ENABLED true
#define DISABLED false 

#define red_light_pin (int)11
#define green_light_pin (int)10
#define blue_light_pin (int)9

#define echoPin 2 
#define trigPin 3

#define servoPin 6
#define servoButtonPin 7

#define temperaturePin A3
#define BETA (float)3950

Servo myservo;
int32_t pos = 1;

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

class Button{
  
  static constexpr byte dTime = 30;
  const bool active;
  bool lastButtonState = HIGH;
  byte lastdTime = 0;

  public:
    bool state = ENABLED;  
    const byte buttonPin;

  public:
  // The constructor takes the GPIO as parameter.
    Button(byte attachTo, bool active = LOW) : buttonPin(attachTo), active(active){}
    void begin(){
      if (active == LOW)
        pinMode(buttonPin, INPUT_PULLUP);
      else
        pinMode(buttonPin, INPUT);
    }

    bool wasPressed(){
      bool buttonState = LOW;
      byte reading = LOW;
      if (digitalRead(buttonPin) == active)
        reading = HIGH;
      if (((millis() & 0xFF) - lastdTime) > dTime)
      {
        if (reading != lastButtonState && lastButtonState == LOW)
          buttonState = HIGH;
        lastdTime = millis() & 0xFF;
        lastButtonState = reading; 
      }
      return buttonState;
    }

    void lock(){
      state = DISABLED;
    }

    void unlock(){
      state = ENABLED;
    }

};

class Display{
  protected:
    Button showNext;
    Button showPrevious;
    Button lockButton;
    LiquidCrystal_I2C lcd; 
    RTC_DS1307 rtc;

    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    typedef struct TIME{
      byte month, day, hour, minute;
    };
    TIME Time;

    String menu[MENUSIZE] = {"Date: ", "Current Time: ", "Page Three \nData3", "Page Four \nData4"};
    byte currentMenuPage = 0;
    public:
      Display(byte nextPin, byte previousPin, byte lockPin, byte lcdAddr, byte cols, byte rows) : showNext{nextPin},  showPrevious{previousPin}, lockButton{lockPin}, lcd(lcdAddr, cols, rows) {}

      void begin()
      {
        showNext.begin();
        showPrevious.begin();
        lockButton.begin();
        lcd.init();                      // initialize the lcd
        // Print a message to the LCD.
        lcd.backlight();
        if (! rtc.begin()) {
          Serial.println("Couldn't find RTC");
          Serial.flush();
          abort();
        }
        showIdle();
      }

      void showIdle(){
        lcd.clear();
        lcd.print("      WELCOME");
      }

      unsigned long durationCalculator(){
        bool started = false;
        unsigned long startTime = 0, endTime = 0;

        do{
          if (!started && digitalRead(lockButton.buttonPin) == LOW){
            started = true;
            startTime = millis();
          }
          
          if (started && digitalRead(lockButton.buttonPin) == HIGH)
          {
            endTime = millis();
            started = false;
          }

        }while(digitalRead(lockButton.buttonPin) == LOW);
        if (endTime == 0){
          Serial.println("Something went wrong;");
          return 0;
        }
        else{
          /*Serial.print("STARTIME: ");
          Serial.println(currentTime);
          Serial.print("ENDTIME: ");
          Serial.println(endTime);
          Serial.print("DURATION IN CALC: ");
          Serial.println(duration);*/
          return endTime - startTime;
        }  
      }

      void ButtonLocker(){
        unsigned long duration = durationCalculator();
        if (showNext.state == ENABLED){
          showNext.lock();
          showPrevious.lock();
        }
        if (duration >= 2000){
          if (showNext.state == DISABLED){
            Serial.println("BUTTONS UNLOCKED");
            showNext.unlock();
            showPrevious.unlock();
          }
        }
      }

      void getTime(){
        DateTime currentTime = rtc.now();
        Time.hour = currentTime.hour();
        Time.minute = currentTime.minute();
        Time.day = currentTime.day();
        Time.month = currentTime.month();
      }

      void printDate(){
        getTime();
        lcd.print(Time.day);
        lcd.print("/");
        lcd.print(Time.month);
      }

      void printTime(){
        getTime();
        lcd.print(Time.hour);
        lcd.print(":");
        lcd.print(Time.minute);
      }

      void update(){
        if (lockButton.wasPressed()){
          Serial.println("\nLOCK BUTTON WAS PRESSED!\n");
          ButtonLocker();
        }

        if (showNext.wasPressed() && showNext.state == ENABLED){
          lcd.clear();     
          if (currentMenuPage >= MENUSIZE) currentMenuPage = 0;
          Serial.println(currentMenuPage);
          switch (currentMenuPage) {
            case 0:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              ++currentMenuPage;
              printDate();
              break;
            case 1:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              ++currentMenuPage;
              printTime();
              break;
            case 2:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              ++currentMenuPage;
              break;
            case 3:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              ++currentMenuPage;
              break;
          }
        }

        if (showPrevious.wasPressed() && showPrevious.state == ENABLED){
          lcd.clear();     
          if (currentMenuPage >= 255) 
          {
            currentMenuPage = MENUSIZE - 1;
            DEBUG;
          }

          Serial.println(currentMenuPage);
          switch (currentMenuPage) {
            case 0:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              --currentMenuPage;
              printDate();
              break;
            case 1:
              lcd.clear(); 
              lcd.print(menu[currentMenuPage]);
              currentMenuPage--;
              printTime();
              break;
            case 2:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              --currentMenuPage;
              break;
            case 3:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              --currentMenuPage;
              break;
          }
        }
      }
};

void moveServo(){

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

void setLeds(){
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
}

void setSensors(){
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
}

void setServo(){
  myservo.attach(servoPin);
  myservo.write(90);
  
}

Display display{13, 8, 4, 0x25, 20, 4};

Button servoButton(servoButtonPin);
long duration;
int distance;
int analogValue;
float temperature;

void setup()
{
  //Serial.begin(115200);
  Serial.begin(9600); 
  
  setLeds();
  setSensors();
  setServo();

  display.begin();
}

int getDistance(){
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  return duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)

}

void setRGBColor(){
  if (distance > 0 && distance < 100)
  	RGB_color(0, 255, 0); // Red
  else if (distance >= 100 && distance < 200)
    RGB_color(255,255,0);
   else if(distance >= 200 && distance < 300)
    RGB_color(255,0,0);
}

float getTemperature(){
  analogValue = analogRead(temperaturePin);
  return 1 / (log(1 / (1023. / analogValue - 1)) / BETA + 1.0 / 298.15) - 273.15;
}

void loop()
{
  display.update();

  distance = getDistance();
  temperature = getTemperature();
  if (temperature > 30)
    Serial.println("WARNING");
  
  //Serial.print("Distance: ");
  //Serial.print(distance);
  //Serial.println(" cm");

  // Serial.print("Temperature: ");
  // Serial.print(temperature);
  // Serial.println(" ℃");

  if(servoButton.wasPressed())
    moveServo();
  
  setRGBColor();
}
