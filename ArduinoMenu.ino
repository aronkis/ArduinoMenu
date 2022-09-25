/* https://wokwi.com/projects/339145759042568786 */
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h> // for the RTC DS1307
//#include "HX711.h" // for the weight sensor
#include <Servo.h>

LiquidCrystal_I2C lcd(0x26,16,2);
RTC_DS1307 rtc;
// HX711 scale;
Servo myservo;

struct Times{
  uint8_t hours, minutes;
};

String Options[6][6] = {
  {"1. Info",       "2. History",          "3. Settings"},
  {"Time & Date:",  "Pet feed at: @Time1", "Time"},
  {"Feed times",    "Pet feed at: @Time2", "Date"},
  {"Treat Amount:", "Pet feed at: @Time3", "Feed Time"},
  {"Food Amount:",  "Pet feed at: @Time4", "Food Amount"},
  {"Sync",          "Pet feed at: @Time5", "Treat Amount"}
};

Times times[6];
String dataFromServer[3];

const int ampReaderPin = A0;
const int tempReaderPin = A1;
// const int hx711DTPin = 2;
// const int hx711SCKPin = 3;
const int servoPin = 6;
const int pumpPin = 7;
const int lockButtonPin = 23;
const int treat2ButtonPin = 25;
const int treat1ButtonPin = 27;
const int backButtonPin = 29;
const int forwardButtonPin = 31;
const int enterButtonPin = 33;
const int escapeButtonPin = 35;
const int redPin = 10;
const int greenPin = 8;
const int bluePin = 9;
const int uvLEDPin = 43;
const int echoPinFood = 50;
const int trigPinFood = 51;
const int echoPinWater = 52;
const int trigPinWater = 53;
uint16_t year = 2000;
uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
uint8_t foodAmount = 0, treatAmount = 0;
String temp;
String hours, minutes;
char command;
int currentPosX = 0, currentPosY = 0, currentHistoryPos = 0;
int cursor = 1;
int distance = 0;
long duration = 0;
uint8_t numberOfFeeds = 1;
String package;

bool pumpRunning = 1;
bool dualFeed = 0;
bool lockedButtons = 0;

void pinModeRGB(const int redPin, const int greenPin, const int bluePin){
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void setServo(){
  myservo.attach(servoPin);
  myservo.write(120);  
}

void treatLeft(){
  if (foodLevel()){
    moveServo(90, 0);
    delay(1000);
    moveServo(0, 90);
  }
}

void treatRight(){
  if (foodLevel()){
    moveServo(90, 180);
    delay(1000);
    moveServo(180, 90);
  }
}

int readUltraSound(const int trigPin, const int echoPin){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2; 
  return distance;
}

float readTemperature(){
  float val = analogRead(tempReaderPin);
  return (val / 1024.0) * 500;
}

void checkPump(){
  if (readUltraSound(trigPinWater, echoPinWater) > 9){
    digitalWrite(pumpPin, LOW);
    pumpRunning = 0;
  }
  else if (!pumpRunning){
    digitalWrite(pumpPin, HIGH);
    pumpRunning = 1;
  }
  delay(100);
}

float ampReading(){
  float value = 0, samples = 0, amp = 0;
  for (int i = 0; i < 5; i++){
    samples += analogRead(ampReaderPin);
    delay(3);
  }
  samples /= 10;
  value = ((samples * 5.0 / 1024.0) - 2.5) / 0.185;
  return value;
}


void setRGB(const int red, const int green, const int blue){
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
  delay(100);
}

bool foodLevel(){
  int value = readUltraSound(trigPinFood, echoPinFood); 
  Serial.print("Food level: ");
  Serial.println(value);
  if (value <= 9){
    setRGB(255, 0, 255);
  }
  else if (value > 9 && value < 16){
    setRGB(0, 0, 255);
  }
  else{
    setRGB(0, 255, 255);
  }
  Serial.print("Food level: ");
  Serial.println(value);
  Serial.print("Food level return : ");
  Serial.println(value < 50);
  

  return value < 25;
}

bool bowlState(){
  //return scale.get_units(10)  == 0;
  return true;
}

void sendDataToServer(){
  Serial1.print("SENDING: #");
  package = String(foodAmount);
  package += "; ";
  package += String(treatAmount);
  package += "; ";
  package += String(dualFeed); 
  package += "; ";
  package += numberOfFeeds;
  package += "; ";
  for (int i = 0; i < numberOfFeeds; i++){
    if (times[i].hours < 10){
      package += "0";
    }
    package += times[i].hours;
    package += ":";
    if (times[i].minutes < 10){
      package += "0";
    }
    package += times[i].minutes;
    package += "; ";
  }
  Serial.println(package);
  Serial1.println(package);
}

void saveTime(String time, uint8_t feedingCount){
  uint8_t sepPos = time.indexOf(":");
  times[feedingCount].hours = (time.substring(0, sepPos)).toInt();
  times[feedingCount].minutes = (time.substring(sepPos + 1, 5)).toInt();
}

void formatData(String buffer){
  int dataCount = 0, feedingCount = 0;
  int nextSepPos = 0, prevSepPos = 0;
  nextSepPos = buffer.indexOf(";");
  String dataToSave = buffer.substring(prevSepPos, nextSepPos);
  while (nextSepPos != -1 && nextSepPos != prevSepPos){
    if (dataToSave.indexOf(":") != -1){
      saveTime(dataToSave, feedingCount);
      feedingCount++;
    }
    else{
      dataFromServer[dataCount] = dataToSave;
      dataCount++;
    }
    prevSepPos = nextSepPos + 2;
    nextSepPos = buffer.indexOf("; ", prevSepPos);
    dataToSave = buffer.substring(prevSepPos, nextSepPos);
  }
}

void getData(){
  char buffer[100] = "";
  if (Serial.available())
  {
    Serial.readBytesUntil('\n', buffer, 100);
    if (buffer != ""){
      Serial.print("\nBUFFER = ");
      Serial.println(buffer);
    }
  }

  if (!strcmp(buffer, "treatLeft")){
    Serial.println("Treating Left");
    treatLeft();
  }
  else if (!strcmp(buffer, "treatRight")){
    Serial.println("Treating Right");
    treatRight();
  }
  else if (strlen(buffer) > 2) {  
    Serial.println(buffer);
    formatData(buffer);
    foodAmount = dataFromServer[0].toInt();
    treatAmount = dataFromServer[1].toInt();
    numberOfFeeds = dataFromServer[2].toInt();
  }
}

void PrintTime(int hours, int minutes) {
  if (hours < 10) {
    lcd.print("0");
  }
  lcd.print(hours);
  lcd.print(":");
  if (minutes < 10) {
    lcd.print("0");
  }
  lcd.print(minutes);
  lcd.print(" ");
}

void PrintTime(String text, DateTime now) {
  now = rtc.now();
  lcd.print(text);
  if (now.hour() < 10) {
    lcd.print("0");
  }
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) {
    lcd.print("0");
  }
  lcd.print(now.minute());
  lcd.print(" ");
}

uint8_t changeTime(uint8_t cursor, char command2, int i = -1){
  command2 = '0';
  while (command2 == '0') {
    command2 = getCommand();
  }
  if (command2 == 'd') {
    if (cursor == 1) {
      minute++;
      if (minute > 59) {
        minute = 0;
      }
    }
    else if (cursor == 0) {
      hour++;
      if (hour > 23) {
        hour = 0;
      }
    }
  }
  else if (command2 == 'a') {
    if (cursor == 1) {
      minute--;
      if (minute > 59) {
        minute = 59;
      }
    }
    else if (cursor == 0) {
      hour--;
      if (hour > 23) {
        hour = 23;
      }
    }
  }
  else if (command2 == 's') {
    cursor++;
  }
  else if (command2 == 'w') {
    cursor = 2;
  }
  if (i != -1){
    times[i].hours = hour;
    times[i].minutes = minute;
  }
  return cursor;
}

void PrintAmount(String text, int amount) {
  lcd.clear();
  lcd.print(text);
  lcd.print(amount);
  lcd.print("g");
}

void getTime(uint8_t &hour, uint8_t &minute) {
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
}

void getDate(uint16_t &year, uint8_t &month, uint8_t &day) {
  DateTime now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
}

void prettyPrint(String text) {
  int lineBreak = findChar(text, ':');
  for (int i = 0; i < text.length(); i++) {
    if (i == lineBreak)
      lcd.setCursor(0, 1);
    lcd.write(text[i]);
  }
}

void prettyPrint(String text, int option) {
  int lineBreak = findChar(text, ':');
  lcd.print(text);
  lcd.setCursor(0, 1);
  switch (option) {
    case 1:
      getTime(hour, minute);
      PrintTime(hour, minute);
      getDate(year, month, day);
      printDate(day, month, year);
      break;
    case 2:
      void(0);
      break;
    case 3:
      PrintAmount("Treat Amount:", treatAmount);
      void(0);
    break;
    case 4:
      PrintAmount("Food Amount: ", foodAmount);
      break;
    case 5:
      void(0);
      break;
  }
}

void printNumberOfFeedTimes(uint8_t numberOfFeeds){
  lcd.clear();
  lcd.print("Number of times: ");
  lcd.setCursor(1, 0);
  lcd.print(numberOfFeeds);
}

int findChar(String text, const char charToFind) {
  for (int i = 0; i < text.length(); i++) {
    if (text[i] == charToFind) {
      return i + 2;
    }
  }
  return -1;
}

char getCommand() {
  if (!lockedButtons){
    if (digitalRead(enterButtonPin)) {
      while (digitalRead(enterButtonPin));
      return 's';
    }
    else if (digitalRead(forwardButtonPin)) {
      while (digitalRead(forwardButtonPin));
      return 'd';
    }
    else if (digitalRead(backButtonPin)) {
      while (digitalRead(backButtonPin));
      return 'a';
    }
    else if (digitalRead(escapeButtonPin)) {
      while (digitalRead(escapeButtonPin));
      return 'w';
    }
    else if (digitalRead(treat1ButtonPin)) {
      while (digitalRead(treat1ButtonPin));
      return '1';
    }
    else if (digitalRead(treat2ButtonPin)) {
      while (digitalRead(treat2ButtonPin));
      return '2';
    }
  } 
  if (digitalRead(lockButtonPin)){
    return '3';
  }
  return '0';
}

void intToChar(char* const output, int number) {
  char num;
  int pos = 1;

  while (number) {
    num = (number % 10) + '0';
    output[pos] = num;
    number /= 10;
    pos--;
  }
  output[2] = '\0';
}

void printDate(int day, int month, int year) {
  if (day < 10) {
    lcd.print("0");
  }
  lcd.print(day);
  lcd.print("/");

  if (month < 10) {
    lcd.print("0");
  }
  lcd.print(month);
  lcd.print("/");
  lcd.print(year);
}

int changeDate(int cursor, uint8_t command2){
  command2 = '0';
  while (command2 == '0') {
    command2 = getCommand();
  }
  if (command2 == 'd') {
    if (cursor == 0) {
      day++;
      if (day == 32) {
        day = 1;
      }
    }
    else if (cursor == 1) {
      month++;
      if (month == 13) {
        month = 1;
      }
    }
    else if (cursor == 2 ) {
      year++;
      if (year > 2099) {
        year = 2020;
      }
    }
  }
  else if (command2 == 'a') {
    if (cursor == 0) {
      day--;
      if (day == 0) {
        day = 31;
      }
    }
    else if (cursor == 1) {
      month--;
      if (month == 0) {
        month = 12;
      }
    }
    else if (cursor == 2) {
      year--;
      if (year == 2020) {
        year = 2022;
      }
    }
  }
  else if (command2 == 's') {
    cursor++;
  }
  else if (command2 == 'w') {
    cursor = 3;
  }
  return cursor;
}

void treat(){
    lcd.clear();
      getTime(hour, minute);
      currentHistoryPos++;

      if (currentHistoryPos > 5) {
        currentHistoryPos = 1;
      }
      hours = String(hour);
      minutes = String(minute);
      temp = "Pet treated at: ";
      temp += hours;
      temp += ":";
      temp += minutes;
     
      Options[currentHistoryPos][1] = temp;
      prettyPrint(Options[currentHistoryPos][1]);
      Serial.println(Options[currentHistoryPos][1] );
      Serial.println(Options[2][0] );
      
      Serial.println("Treat given!");
}


void checkMotor(){
  if (ampReading() > 20){
    Serial.println("TO MUCH AMPS!!!");
    while(1);
  }
}

void moveServo(const int from, const int to){
  if (from < to){
    for (int i = from; i <= to; i += 5){
      myservo.write(i);
      checkMotor();
      delay(15);
    }
  }
  else if (from  > to){
    for (int i = from; i > to; i -= 5){
      myservo.write(i);
      checkMotor();
      delay(15);
    }
  }
}

void feedInit(){
  moveServo(120, 0);
  delay(1000);
  moveServo(0, 120);
  if (dualFeed){
    moveServo(120, 180);
    delay(1000);
    moveServo(180, 120);
  }
}

void printFeedTimes(int i){
  lcd.clear();
  lcd.print("Feed time ");
  lcd.print(i + 1);
  lcd.setCursor(0, 1);
  if (times[i].hours < 10){
    lcd.print("0");
  }
  lcd.print(times[i].hours);
  lcd.print(":");
  if (times[i].minutes < 10){
    lcd.print("0");
  }
  lcd.print(times[i].minutes);
}

void setup()
{
  
  Serial.begin(115200);
  Serial1.begin(115200);
  rtc.begin();
  /*scale.begin(hx711DTPin, hx711SCKPin)
    if (scale.is_ready()){
      scale.set_scale();  // remove any object from the scale
      delay(1000);
      scale.tare(); // calibrating the scale
    }
  */
  pinMode(trigPinWater, OUTPUT);
  pinMode(echoPinWater, INPUT); 
  pinMode(trigPinFood, OUTPUT);
  pinMode(echoPinFood, INPUT); 
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, HIGH);
  setServo();
  // //startWaterPump();
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  //rtc.adjust(DateTime(2022, 9, 19, 13, 10, 0));
  lcd.init();
  lcd.backlight(); // no need if LED pads shorted

  pinMode(uvLEDPin, INPUT);
  pinModeRGB(redPin, greenPin, bluePin);
  pinMode(treat2ButtonPin, INPUT);
  pinMode(treat1ButtonPin, INPUT);
  pinMode(forwardButtonPin, INPUT);
  pinMode(backButtonPin, INPUT);
  pinMode(enterButtonPin, INPUT);
  pinMode(escapeButtonPin, INPUT);
  pinMode(lockButtonPin, INPUT);

  setRGB(255,255,255);
  prettyPrint(Options[currentPosY][currentPosX]);
}
int lastHour = 0;
void loop()
{
  DateTime now = rtc.now();
  command = getCommand();
  // sensor readings.
  while (command == '0') {
    command = getCommand();
    checkPump();
    if (now.hour() % 5 == 0 && now.hour() != lastHour){
      lastHour = now.hour();
    }
    for (int i = 0 ; i < numberOfFeeds; i++){  
      if (now.hour() == times[i].hours && now.minute() == times[i].minutes && times[i].hours != 0) {
        if (foodLevel() && bowlState()){
          feedInit();
          Serial.println("FEEDING!");
        }
      }
    }
    getData();

  }
  switch (command) {
    case '1':
      treat();
      Serial.println("Rotating the motor!");
      treatLeft();
      delay(1000);
      lcd.clear();
      if (currentPosY == 0 || currentPosX != 0) {
        prettyPrint(Options[currentPosY][currentPosX]);
      }
      else {
        prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      break;
    case '2':
      treat();
      Serial.println("Rotating the motor!");
      treatRight();
      delay(1000);
      lcd.clear();
      if (currentPosY == 0 || currentPosX != 0) {
        prettyPrint(Options[currentPosY][currentPosX]);
      }
      else {
        prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      break;
    case '3':
      if (!lockedButtons){
        Serial.println("LOCKED");
        lockedButtons = 1;
      }
      else {
        long startTime = millis();
        while (digitalRead(lockButtonPin));
        long endTime = millis();
        if (endTime - startTime > 3000){
          lockedButtons = 0;
          Serial.println("UNLOCKED");
        }
      }
      break;
    case 'd':
      lcd.clear();
      if (currentPosY == 0) { /* In the main menu. */
        if (currentPosX < 2)
          currentPosX++;
        else {
          currentPosX = 0;
        }
      }
      else { /* In the submenus. */
        if (currentPosY < 5) {
          currentPosY++;
        }
        else {
          currentPosY = 1;
        }
      }
      if (currentPosY == 0 || currentPosX != 0) {
        prettyPrint(Options[currentPosY][currentPosX]);
      }
      else {
        prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      break;
    case 'a':
      lcd.clear();
      if (currentPosY == 0) { /* In the main menu. */
        if (currentPosX > 0) {
          currentPosX--;
        }
        else {
          currentPosX = 2;
        }
      }
      else { /* In the submenus. */
        if (currentPosY > 1) {
          currentPosY--;
        }
        else {
          currentPosY = 5;
        }
      }
      if (currentPosY == 0 || currentPosX != 0) {
        prettyPrint(Options[currentPosY][currentPosX]);
      }
      else {
        prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      break;
    case 's':
      if (currentPosY > 0 && currentPosX == 2) {
        char command2;
        int cursor = 0;
        switch (currentPosY) {
          case 1:
            delay(150);
            getTime(hour, minute);
            while (cursor < 2) {
              lcd.clear();
              PrintTime(hour, minute);
              cursor = changeTime(cursor, command2);
            }
            if (currentPosY == 1) {
              now = rtc.now();
              getDate(year, month, day);
              rtc.adjust(DateTime(year, month, day, hour, minute, 0));
            }
          break;

          case 2:
            cursor = 0;
            getDate(year, month, day);
            while (cursor < 3) {
              lcd.clear();
              printDate(day, month, year);
              cursor = changeDate(cursor, command2);
            }
            getTime(hour, minute);
            rtc.adjust(DateTime(year, month, day, hour, minute, 0));
          break;

          /*Dynamic feed time array + UI*/
          case 3:
            
            command2 = '0';
            lcd.clear();
            lcd.print("Number of times: ");
            lcd.setCursor(0, 1);
            lcd.print(numberOfFeeds);
            while (command2 != 's') {
              if (command2 == 'd' && numberOfFeeds < 5) {
                numberOfFeeds++;
                lcd.clear();
                lcd.print("Number of times: ");
                lcd.setCursor(0, 1);
                lcd.print(numberOfFeeds);
              }
              else if (command2 == 'a' && numberOfFeeds > 1) {
                numberOfFeeds--;
                lcd.clear();
                lcd.print("Number of times: ");
                lcd.setCursor(0, 1);
                lcd.print(numberOfFeeds);
              }
              command2 = getCommand();
              //delay(150);
            }
            for (int i = 0; i < numberOfFeeds; i++) {
              cursor = 0;
              while (cursor < 2) {
                lcd.clear();
                lcd.print("Feed time ");
                lcd.print(i + 1);
                lcd.setCursor(0,1);
                PrintTime(hour, minute);
                cursor = changeTime(cursor, command2, i);
                //delay(150);
              }
              times[i].hours = hour;
              times[i].minutes = minute;
              lcd.clear();
              lcd.print("Dual Feed?");
              command2 = '0';
              while (command2 != 'w' && command2 != 's'){
                command2 = getCommand();
              }
              if (command2 == 'w'){
                dualFeed = 0;
              }
              else if (command2 == 's'){
                dualFeed = 1;
              }
            }
          break;
          case 4 ... 5:
            char dummyText[3];
            int amount = 0;
            command2 = '0';
            while (command2 != 's') {
              PrintAmount("Amount: ", amount);
              if (command2 == 'd') {
                if (currentPosY == 4 && amount < 35) {
                  amount += 5;
                }
                if (currentPosY == 5 && amount < 15) {
                  amount += 5;
                }
              }
              else if (command2 == 'a' && amount >= 5) {
                amount -= 5;
              }
              command2 = '0';
              while (command2 == '0') {
                command2 = getCommand();
              }
            }
            if (currentPosY == 4) {
              foodAmount = amount - 5;
            }
            else if (currentPosY == 5) {
              treatAmount = amount - 5;
            }
            break;
        }
      }
      else if (currentPosY == 2 && currentPosX == 0){
          int i = 0;
          lcd.clear();
          lcd.print("Feed time ");
          lcd.print(i + 1);
          lcd.setCursor(0, 1);
          if (times[i].hours < 10){
            lcd.print("0");
          }
          lcd.print(times[i].hours);
          lcd.print(":");
          if (times[i].minutes < 10){
            lcd.print("0");
          }
          lcd.print(times[i].minutes);
          char command2 = '0';
          while(command2 != 'w') {
            if (command2 == 'd'){
              i++;
              if (i == numberOfFeeds){
                i = 0;
              }
              printFeedTimes(i);
            } 
            if (command2 == 'a'){
              i--;
              if (i < 0){
                i = numberOfFeeds - 1;
              }
              printFeedTimes(i);
            }
            
            command2 = getCommand();
          }
          prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      else if (currentPosY == 5 && currentPosX == 0){
        Serial.println("SYNCING...");
        sendDataToServer();
      }
      else  if (currentPosY == 0) {
        currentPosY = 1;
      }
      
      lcd.clear();
      if (currentPosX == 0) {
        prettyPrint(Options[currentPosY][currentPosX], currentPosY);
      }
      else {
        prettyPrint(Options[currentPosY][currentPosX]);
      }

      break;

    case 'w':
      currentPosY = 0;
      lcd.clear();
      prettyPrint(Options[currentPosY][currentPosX]);
      break;
  }
}
