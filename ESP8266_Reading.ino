#include <WiFiManager.h>
#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>
#include <wm_strings_es.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//secret information
const char* ssid = "DIGI_42efb8";
const char* password = "5932bd10";
const char* serverUrl = "http://192.168.1.7:5000/post-data";

const int LOPlusPin = D1; //LO+ connected to D1
const int LOMinusPin = D2; //LO- connected to D2


//global variables 
const int batchSize = 5000;
int readIndex = 0;
const char* eqID = "ESP8266-01";
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
  pinMode(A0, INPUT); //Output connected to A0
  pinMode(LOPlusPin, INPUT);
  pinMode(LOMinusPin, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // If WiFi cannot be established, pause
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize a NTPClient to get time
  timeClient.begin();

  //the first time it gets information takes longer
  timeClient.update();
}

void loop() {
  int ecgValue;

  if (WiFi.status() == WL_CONNECTED) {

    if (readIndex >= batchSize) {

           
      jsonString +="]}";

      Serial.println(jsonString);

      sendEcgData();

      // Reset index for readings
      readIndex = 0;
    }
    else{


      if(readIndex==0){
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
      
      ecgValue=readEcg();

      jsonString += String(ecgValue);

      readIndex++;
      delay(10);

    }
  }
}

//this function syncs the date and time and also return the timestamp
String syncDateAndTime(){
  timeClient.update();

  // returns an unsigned long with the epoch time (number of seconds that have elapsed since January 1, 1970 (midnight GMT);
  time_t epochTime = timeClient.getEpochTime();

  // Convert to milliseconds (if needed)
  unsigned long long epochTimeMs = epochTime * 1000 + millis() % 1000;

  return String(epochTimeMs);
}


int readEcg() {
  int ecgValue;

  if (digitalRead(LOPlusPin) == HIGH || digitalRead(LOMinusPin) == HIGH) {
    Serial.println("Lead off detected!");
    ecgValue=-1;
  } else {
    ecgValue = analogRead(A0);
  }

  return ecgValue;
}





void sendEcgData() {

  http.begin(wifiClient, serverUrl); // Initialize HTTPClient
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonString); // Send the JSON string

  if (httpResponseCode != 200) {
    Serial.println("Error code: " + String(httpResponseCode));
   
    delay(5000); // Delay for error
  }

  http.end(); // Close connection
}
