#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>


const char* ssid = "ssid";
const char* password = "password";
const char* serverUrl = "serverURL";

const int LOPlusPin = D1; //LO+ connected to D1
const int LOMinusPin = D2; //LO- connected to D2

const int batchSize = 5000;  // 1159us to read EKG, so ~52k for 1min
int readIndex = 0;
String postData = "{\"ekgData\": ["; // Start the JSON string

WiFiClient wifiClient;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT); //Output connected to A0
  pinMode(LOPlusPin, INPUT);
  pinMode(LOMinusPin, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); //if wifi can not be established pause
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    readEKG();

    if (readIndex >= batchSize) {

      sendEKGData();

    } else {
      delay(1);
    }
  }
}

void readEKG() {
  int ecgReading = -1;
  if (digitalRead(LOPlusPin) == HIGH || digitalRead(LOMinusPin) == HIGH) {
    Serial.println("Lead off detected!");
  } else {
    ecgReading = analogRead(A0);
  }

  // Append reading to postData // NOTE: probably takes enough time to not need another delay
  if (readIndex > 0) {
    postData += ", ";
  }
  postData += String(ecgReading);

  readIndex++;
}

void sendEKGData() {
  postData += "]}"; // Complete the JSON string

  http.begin(wifiClient, serverUrl); // Initialize HTTPClient
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(postData);
  if (httpResponseCode != 200){
    Serial.println(httpResponseCode);
    Serial.println(postData);
    delay(5000);
  }

  http.end();

  readIndex = 0;
  postData = "{\"ekgData\": ["; // Reset the JSON string
}
