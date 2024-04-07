#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//secret information
const char* ssid = "Delya";
const char* password = "delya1985";
const char* serverUrl = "http://192.168.1.11:5000/post-data";

const int LOPlusPin = D1; //LO+ connected to D1
const int LOMinusPin = D2; //LO- connected to D2


//global variables 
const int batchSize = 1000;
int readIndex = 0;
const char* eqID = "ESP8266-02";
String ecgValuesJson;
String timeValuesJson;
String timestamp;


//http server
WiFiClient wifiClient;
HTTPClient http;

// define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//variables for syncing the time
unsigned long syncMillis;

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
 
  timeClient.setTimeOffset(10800); //Romania is GTM + 3 => 3*3600 = 3*1hour

  //the first time it gets information takes longer
  timeClient.update();
}

void loop() {
  int ecgValue;
  unsigned long timeValue;
  if (WiFi.status() == WL_CONNECTED) {

    if (readIndex >= batchSize) {

           
      ecgValuesJson +="]";
      timeValuesJson +="]";

      Serial.println(timeValuesJson);
      Serial.println(ecgValuesJson);

      sendEcgData();

      // Reset index for readings
      readIndex = 0;
    }
    else{


      if(readIndex==0){
        timestamp=syncDateAndTime();
        ecgValuesJson="[";
        timeValuesJson="[";
      
      }
      else{
        ecgValuesJson += ",";
        timeValuesJson += ",";
      }
      
      timeValue=millis()-syncMillis;
      ecgValue=readEcg();

      ecgValuesJson += String(ecgValue);
      timeValuesJson += String(timeValue);

      readIndex++;
      delay(30);

    }
  }
}

//this function syncs the date and time and also return the timestamp
String syncDateAndTime(){
  timeClient.update();

  // returns an unsigned long with the epoch time (number of seconds that have elapsed since January 1, 1970 (midnight GMT);
  time_t epochTime = timeClient.getEpochTime();

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int day = ptm->tm_mday;

  //tm_mon starts at 0, we add 1 to the month so that January corresponds to 1
  int month = ptm->tm_mon+1;

  //we need to add 1900 because the tm_year saves the number of years after 1900
  int year = ptm->tm_year+1900;

    int hour = ptm->tm_hour;
    int minute = ptm->tm_min;
    int second = ptm->tm_sec;

    // Update syncMillis with the current millis() to keep track of the time since last sync
    syncMillis = millis();

    // Format the timestamp as "YYYYMMDD HH:MM:SS"
    char timestamp[20]; 
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    return String(timestamp);
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
  //make the final Json Structure
    String json = "{\"initialTimestamp\": \"" + timestamp + "\",";
    json += "\"eqID\": \"" + String(eqID) + "\",";
    json += "\"ecgValues\": " + ecgValuesJson + ",";
    json += "\"elapsedTimeValues\": " + timeValuesJson;
    json += "}";


  http.begin(wifiClient, serverUrl); // Initialize HTTPClient
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(json); // Send the JSON string
  Serial.println(json);
  if (httpResponseCode != 200) {
    Serial.println("Error code: " + String(httpResponseCode));
   
    delay(5000); // Delay for error
  }

  http.end(); // Close connection
}
