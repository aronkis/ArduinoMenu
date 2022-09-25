#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char *ssid = "Redmi Note 10 5G";
const char *password = "danana123";

String dataFromServer[4];
String feedingTimes[6];
String request, data;
String DataFromServer[10];
WiFiClient client;


int getFeedTimes(String payload){
  int nextIndex = 0, prevIndex = 0, count = 0, valueIndex = 0;
  nextIndex = payload.indexOf("feedTime");
  while(nextIndex != prevIndex && nextIndex != -1){ 
    valueIndex = payload.indexOf("value=", nextIndex);
    feedingTimes[count] = payload.substring(valueIndex + 7, valueIndex + 12);
    count++;
    prevIndex = nextIndex;
    nextIndex = payload.indexOf("feedTime", prevIndex + 1);
  }
  return count;
}

// Search for DualFeed
void getDataFromServer()
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;                                                // Declare an object of class HTTPClient
    http.begin(client, "https://catfeeder.pythonanywhere.com/"); // Specify request destination
    uint16_t httpCode = http.GET();                                 // Send the request
    if (httpCode > 0)
    { // Check the returning code
        String payload = http.getString();
        int treatLeftPos = payload.indexOf("treatLeftMarker") + 24;
        int treatRightPos = payload.indexOf("treatRightMarker") + 25;
        int foodAmountPos = payload.indexOf("Food amount: ") + 16;
        int treatAmountPos = payload.indexOf("Treat amount: ") + 17;
        int syncOrderPos = payload.indexOf("syncOrder") + 18;
        int feedModePos = payload.indexOf("Dual Feed");
        //uint16_t currentTimePos = payload.indexOf("Current time: ") + 17;
        //uint16_t currentDatePos = payload.indexOf("Current date: ") + 17;
        String foodAmount = payload.substring(foodAmountPos, foodAmountPos + 2);
        String treatAmount = payload.substring(treatAmountPos, treatAmountPos + 2);
        String numberOfFeedTimes = String(getFeedTimes(payload));
        String treatLeftValue = payload.substring(treatLeftPos, treatLeftPos + 1);
        String treatRightValue = payload.substring(treatRightPos, treatRightPos + 1);
        String syncOrderValue = payload.substring(syncOrderPos, syncOrderPos + 1);
        if (treatLeftValue == "1"){
          treatLeft();
        }
        if (treatRightValue == "1"){
          treatRight();
        }
        if (syncOrderValue == "1"){
          sendData();
        } 
        if (feedModePos != -1){
          dataFromServer[3] = "DualFeed";
        }
        else{
          dataFromServer[3] = "SingleFeed";
        }
        //String currentTime = payload.substring(currentTimePos, currentTimePos + 8);
        //String currentDate = payload.substring(currentDatePos, currentDatePos + 10);
        dataFromServer[0] = foodAmount;
        dataFromServer[1] = treatAmount;
        dataFromServer[2] = numberOfFeedTimes;
    }
    http.end();
  }
}

void formatData(String buffer){
  buffer = buffer.substring(buffer.indexOf("#") + 1);
  int prevSepPos = 0, count = 0;
  int nextSepPos = buffer.indexOf(";");
  data = buffer.substring(prevSepPos, nextSepPos);
  prevSepPos = nextSepPos + 2;
  DataFromServer[count] = data;

  while(nextSepPos != -1 && nextSepPos != prevSepPos){
    nextSepPos = buffer.indexOf(";", prevSepPos);
    data = buffer.substring(prevSepPos, nextSepPos);
    prevSepPos = nextSepPos + 2;
    count++;
    DataFromServer[count] = data;
  }
}

void treatLeft(){
  Serial.println("Rotating Left");
  Serial1.print("treatLeft");
  setLeftTreat();
}

void treatRight(){
  Serial.println("Rotating Right");
  Serial1.print("treatRight");
  setRightTreat();
}

void setLeftTreat(){
  HTTPClient http;                                               
  http.begin(client, "http://catfeeder.pythonanywhere.com/info?treatLeft=0");
  uint16_t httpCode = http.GET();
  http.end();                
}

void setRightTreat(){
  HTTPClient http;
  http.begin(client, "http://catfeeder.pythonanywhere.com/info?treatRight=0");
  uint16_t httpCode = http.GET();
  http.end();                
}

void setSync(){
  HTTPClient http;
  http.begin(client, "https://catfeeder.pythonanywhere.com/info?syncFlag=0");
  uint16_t httpCode = http.GET();
  http.end();  
}

void getData(){
  char buffer[50] = "";
  if (Serial.available())
  {
    Serial.readBytesUntil('\n', buffer, 50);
    Serial.print("Buffer: ");
    Serial.println(buffer);
    formatData(buffer);
  }

  request = "https://catfeeder.pythonanywhere.com/settings?feedAmount=";
  request += DataFromServer[0];
  request += + "&treatAmount=";
  request += DataFromServer[1];
  request += "&dualFeed=";
  request += DataFromServer[2];
  request += + "&numberOfFeedTimes=";
  request += DataFromServer[3];
  request += + "&feedSchedules=";
  for (int i = 0; i < DataFromServer[2].toInt(); i++){
    request += DataFromServer[i + 4];
    request += ",";
  }
  request.remove(request.length() - 1, 1);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(client, request);
    uint16_t httpCode = http.GET();
    Serial.println(httpCode);
    http.end();
  }
  delay(2000);
}

void sendData(){
    Serial.println("GETTING DATA FROM THE SERVER....");
    for (int i = 0; i < 4; i++){
      Serial.print(dataFromServer[i]);
      Serial.print("; ");
      Serial1.print(dataFromServer[i]);
      Serial1.print("; ");
    }
    for (int i = 0; i < dataFromServer[2].toInt(); i++){
      Serial.print(feedingTimes[i]);
      Serial.print("; ");
      Serial1.print(feedingTimes[i]);
      Serial1.print("; ");
    }
    Serial1.println();
    setSync();
}


void setup()
{
    Serial1.begin(115200);
    Serial.begin(115200);
    Serial.println("Starting...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(3000);
        Serial.println("Connecting..");
    }
    Serial.println("Connected to WiFi Network");
}

void loop()
{
  //sendData();
  
  getData();
  getDataFromServer();
  delay(3000);
}
