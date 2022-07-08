#include<LiquidCrystal_I2C.h>

#define DEBUG Serial.print("\nIN: "); Serial.print(__func__); Serial.print(" ON LINE: "); Serial.print(__LINE__)
#define MENUSIZE 4
#define ENABLED true
#define DISABLED false

unsigned long currentMillis;  

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
      DEBUG;
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
    static const byte clearAfter = 3;      
    uint32_t previousMillis = 0;
    unsigned long duration;

    String menu[MENUSIZE] = {"Page ONE \nData1", "Page Two \nData2", "Page Three \nData3", "Page Four \nData4"};
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
        showIdle();
      }

      void showIdle(){
        lcd.clear();
        lcd.print(menu[currentMenuPage]);
      }

      unsigned long durationCalculator(){
        unsigned long duration = 0;
        bool started = false;
        unsigned long currentTime = 0, endTime = 0;

        do{
          if (!started && digitalRead(lockButton.buttonPin) == LOW){
            started = true;
            currentTime = millis();
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
          duration = endTime - currentTime;
          return duration;
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

      void update(){
        if (lockButton.wasPressed()){
          Serial.println("\nLOCK BUTTON WAS PRESSED!\n");
          ButtonLocker();
        }

        if (showNext.wasPressed() && showNext.state == ENABLED){
          lcd.clear();     
          if (currentMenuPage == MENUSIZE - 1) currentMenuPage = -1;
          lcd.print(menu[++currentMenuPage]);
        }

        if (showPrevious.wasPressed() && showNext.state == ENABLED){
          lcd.clear();
          if (currentMenuPage == 0) currentMenuPage = MENUSIZE;
          lcd.print(menu[--currentMenuPage]);
        }
      }
};

Display display{A0, A1, A2, 0x25, 20, 4};

void setup()
{
  Serial.begin(115200);
  display.begin();
}

void loop()
{
  display.update();
}
