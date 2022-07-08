#include<LiquidCrystal_I2C.h>
#include "RTClib.h"

#define DEBUG Serial.print("\nIN: "); Serial.print(__func__); Serial.print(" ON LINE: "); Serial.print(__LINE__)
#define MENUSIZE 4
#define ENABLED true
#define DISABLED false 

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
        lcd.print(menu[currentMenuPage]);
        printDate();
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
              printDate();
              currentMenuPage++;
              break;
            case 1:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              printTime();
              currentMenuPage++;
              break;
            default:
              lcd.print(menu[currentMenuPage]);
              currentMenuPage++;
              break;
          }
        }

        if (showPrevious.wasPressed() && showPrevious.state == ENABLED){
          lcd.clear();     
          if (currentMenuPage >= 255) currentMenuPage = MENUSIZE - 1;
          Serial.println(currentMenuPage);
          switch (currentMenuPage) {
            case 0:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              printDate();
              currentMenuPage--;
              break;
            case 1:
              lcd.clear();
              lcd.print(menu[currentMenuPage]);
              printTime();
              currentMenuPage--;
              break;
            default:
              lcd.print(menu[currentMenuPage]);
              currentMenuPage--;
              break;
          }
        }
      }
};

Display display{13, 7, 4, 0x25, 20, 4};

void setup()
{
  Serial.begin(115200);
  display.begin();
}

void loop()
{
  display.update();
}
