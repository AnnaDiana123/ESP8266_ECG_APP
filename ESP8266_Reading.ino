#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>


const int LOPlusPin = D1; //LO+ connected to D1
const int LOMinusPin = D2; //LO- connected to D2

//global variables 
const char* eqID = "ESP8266-01";
const int batchSize = 5000;
int readIndex = 0;
String jsonString;
String timestamp;

//http server
WiFiClient wifiClient;
HTTPClient http;

// define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


void setup() {

  Serial.begin(115200);
  pinMode(LOPlusPin, INPUT);
  pinMode(LOMinusPin, INPUT);

  //establish wifi connection
  connectToWiFi();

  //initialize a NTPClient to get time
  timeClient.begin();

  //the first time it gets the time it takes longer
  timeClient.update();
}

void loop() {
  //if it is connected to wifi
  if (WiFi.status() == WL_CONNECTED) {
    if (readIndex >= batchSize) {
      //end json string
      jsonString +="]}";

      Serial.println(jsonString);

      sendEcgData();

      // Reset index for readings
      readIndex = 0;
    }
    else{
      if(readIndex==0){
        //sync with ntp server to get time
        timestamp=syncDateAndTime();
        Serial.println(timestamp);

        //initialize json String
        jsonString="{\"initialTimestamp\": \"" + timestamp + "\",";
        jsonString += "\"eqID\": \"" + String(eqID) + "\",";
        jsonString += "\"ecgValues\": [";
      
      }
      else{
        jsonString += ",";
      }
      
      int ecgValue=readEcg();

      jsonString += String(ecgValue);

      readIndex++;
      //add 10 milliseconds delay, the reading frequenzy is 100Hz
      delay(10);
    }
  }
  else{ //if it is not connected to wifi
    //try reconnecting to wifi
    connectToWiFi();
  }
}

void connectToWiFi(){
  //WiFiManager, Local intialization, once the wifi is connected, there is no need to keep it around
  WiFiManager wm;

  //set timeout in seconds, it will try to connect to previous wifi libraries for 1 min
  wm.setConfigPortalTimeout(60); 

  //after 1 min it will open congiguration portal
  if (! wm.autoConnect("ECGDevice","ESP8266")) {
    Serial.println("Failed to connect to WiFi. Opening configuration portal.");
  }

  Serial.println("Connected to WiFi");
}

//this function syncs the date and time and also return the timestamp
String syncDateAndTime(){
  timeClient.update();

  // returns an unsigned long with the epoch time (number of seconds that have elapsed since January 1, 1970 (midnight GMT);
  time_t epochTime = timeClient.getEpochTime();

  // Convert to milliseconds and sync with internal millis()
  unsigned long long epochTimeMs = epochTime * 1000 + millis() % 1000;

  return String(epochTimeMs);
}


int readEcg() {
  int ecgValue;

  if (digitalRead(LOPlusPin) == HIGH || digitalRead(LOMinusPin) == HIGH) { // if patches are not connected
    Serial.println("Lead off detected!");
    ecgValue=-1;
  } else {
    //read ecg value
    ecgValue = analogRead(A0);
  }

  return ecgValue;
}


void sendEcgData() {
  String serverUrl = "http://192.168.1.11:5000/post-data";
  Serial.println(serverUrl);
  //initialize HTTPClient
  http.begin(wifiClient, serverUrl); 
  http.addHeader("Content-Type", "application/json");

  //send the JSON string
  int httpResponseCode = http.POST(jsonString); 

  if (httpResponseCode != 200) {
    Serial.println("Error code: " + String(httpResponseCode));
   
    delay(5000); //delay for error
  }

  http.end(); //close connection
}
