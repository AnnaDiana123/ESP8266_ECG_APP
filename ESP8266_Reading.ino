#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//secret information
const char* ssid = "ssid";
const char* password = "password";
const char* serverUrl = "serverURL";

const int LOPlusPin = D1; //LO+ connected to D1
const int LOMinusPin = D2; //LO- connected to D2


//global variables 
const int batchSize = 2000;
int readIndex = 0;
String postData = "{\"ekgData\": [";
const char* eqID = "ESP8266-01";

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
 
  timeClient.setTimeOffset(7200); //Romania is GTM + 2 => 2*3600 = 2*1hour

  //the first time it gets information takes longer
  timeClient.update();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    if (readIndex >= batchSize) {
      String startReadingTime=getDateAndTime();

      // Complete building the JSON string
      postData += "]";
      postData += ", \"startReadingTime\": \"" + startReadingTime + "\"";
      postData += ", \"eqID\": \"" + String(eqID) + "\"}";
      
      sendEKGData();

      // Reset index for readings
      readIndex = 0;
      postData = ""; // Clear postData for next batch

      // Reset reading start time and start building the JSON string
      postData = "{\"ekgData\": [";
    }

    readEKG();
  }
}

String getDateAndTime(){
  timeClient.update();

  // returns an unsigned long with the epoch time (number of seconds that have elapsed since January 1, 1970 (midnight GMT);
  time_t epochTime = timeClient.getEpochTime();

  //get time
  String formattedTime = timeClient.getFormattedTime(); 

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int currentDay = ptm->tm_mday;

  //tm_mon starts at 0, we add 1 to the month so that January corresponds to 1
  int currentMonth = ptm->tm_mon+1;

  //we need to add 1900 because the tm_year saves the number of years after 1900
  int currentYear = ptm->tm_year+1900;

  //Print complete date:
  String currentDateAndTime = String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear) + " " + formattedTime;
  
  return currentDateAndTime;
}

void readEKG() {
  int ecgReading = -1;
  if (digitalRead(LOPlusPin) == HIGH || digitalRead(LOMinusPin) == HIGH) {
    Serial.println("Lead off detected!");
  } else {
    ecgReading = analogRead(A0);
    // Add the reading to postData, no comma needed as it's handled in loop()
    if (readIndex > 0) {
      postData += ", ";
    }
    postData += String(ecgReading);
    readIndex++;
  }

  delay(10);
}

void sendEKGData() {
  http.begin(wifiClient, serverUrl); // Initialize HTTPClient
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(postData); // Send the JSON string
  if (httpResponseCode != 200) {
    Serial.println("Error code: " + String(httpResponseCode));
    Serial.println(postData);
    delay(5000); // Delay for error
  }

  http.end(); // Close connection
}
