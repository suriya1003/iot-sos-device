#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//q#include <PulseSensorPlayground.h>
#include "secrets.h"

#define AWS_IOT_PUBLISH_TOPIC   "esp8266/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp8266/sub"
#define PUSH_BUTTON_PIN 0  // Pin to which the push button is connected
#define HRPIN A0
#define PIEZOIN D4
#define GEOFENCE_LATITUDE   12.99006939   // Latitude of the geofence area
#define GEOFENCE_LONGITUDE  80.21110535 // Longitude of the geofence area
#define GEOFENCE_RADIUS     1000      // Radius of the geofence area in meters
#define MONITOR_INTERVAL    60000    // 10 minutes in milliseconds


WiFiClientSecure net;
BearSSL::X509List cert(CA_CERT);
BearSSL::X509List client_crt(CLIENT_CERT);
BearSSL::PrivateKey key(PRIVATE_KEY);
PubSubClient client(net);

TinyGPSPlus gps;
SoftwareSerial GPS_SoftSerial(D5, D6); // (Rx, Tx)

float altitude;
uint8_t hr_val;
uint8_t min_val;
uint8_t sec_val;
char time_str[50];
bool buttonPressed = false;
bool outOfGeofence = false;
unsigned long lastMonitorTime = 0;
const char outmsg[] = "User is out of geofence!!";               //TAMIM2.4G
const char inmsg[] = "User is inside geofence";               //TAMIM2.4G
bool flag;

const int sensorPin = A0; // Heart rate sensor pin
int heartrate = 0; // Current heart rate
unsigned long lastHeartRateCheck = 0; // Last time heart rate was checked
int heartrateThreshold = 100; // Threshold heart rate value


static unsigned long lastFallTime = 0;
static bool waitingForRecovery = false;
int piezoval;
unsigned long fallDetectionTimer = 0;
const unsigned long fallDetectionInterval = 500; 



void NTPConnect(void) {
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 1510592825) { // Use your desired time here
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void connectAWS() {
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  NTPConnect();

  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}
bool isWithinGeofence(float latitude, float longitude) {
  float distance = gps.distanceBetween(latitude, longitude, GEOFENCE_LATITUDE, GEOFENCE_LONGITUDE);
  return distance <= GEOFENCE_RADIUS;
}

void publishSosLocationMessage(float latitude, float longitude, unsigned long altitude, uint8_t hr_val, uint8_t min_val, uint8_t sec_val) {
  JsonDocument doc;
  sprintf(time_str, "%02d:%02d:%02d", hr_val, min_val, sec_val);

  doc["time"] = time_str;
  doc["latitude"] = latitude;
  doc["longitude"] = longitude;
  doc["altitude"] = altitude;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
void publishGeoFenceMessgage() {
  JsonDocument doc;

  if(flag==false){
  doc["message"] = outmsg;
  }
  else{
      doc["message"] = inmsg;

  }
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
void publishFallMessage(float latitude, float longitude){
    JsonDocument doc;
    doc["message"] = "HELP! I'VE FALLEN ";
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;
    char jsonBuffer[512];
   serializeJson(doc, jsonBuffer);
   client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

}
void publishFallMessage2(){
    JsonDocument doc;
    doc["message"] = "HELP! I'VE FALLEN ";
    char jsonBuffer[512];
   serializeJson(doc, jsonBuffer);
   client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

}
void publishHeartRateAlert(int heartRate) {
  DynamicJsonDocument doc(128);
  doc["message"] = "Heart rate exceeds threshold!";
  doc["heart_rate"] = heartRate;
  char jsonBuffer[128];
  serializeJson(doc, jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Heart rate alert published to AWS IoT topic");
}
void setup() {
  Serial.begin(115200);
  GPS_SoftSerial.begin(9600);
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIEZOIN,INPUT_PULLUP);
  connectAWS();
}

void loop() {
  //FALL DETECTION CODE 
  if (millis() - fallDetectionTimer > fallDetectionInterval){
  fallDetectionTimer = millis();
  piezoval=digitalRead(PIEZOIN);
  Serial.print("Fall detected : ");
  Serial.println(piezoval);
  Serial.print("Is waiting for recovery");
  Serial.println(waitingForRecovery);
  
  if (piezoval == LOW && !waitingForRecovery ) {
    Serial.println("!!!inside fallen code !!!");
    lastFallTime = millis();
    waitingForRecovery = true;
     // Update the fall detection timer

    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      unsigned long altitude = (gps.altitude.isValid()) ? gps.altitude.meters() : 0;
      uint8_t hr_val = gps.time.hour();
      uint8_t min_val = gps.time.minute();
      uint8_t sec_val = gps.time.second();
      
      publishFallMessage(latitude, longitude);
    }
    else {
      publishFallMessage2();
  }
  }
  if (waitingForRecovery && (millis() - lastFallTime > 120000)) {
    Serial.println("Resetting wait for recovery");

    waitingForRecovery = false;
  }
  }

// heart rate

// if (millis() - lastHeartRateCheck >= 1000) {
//     lastHeartRateCheck = millis();
    
//     // Read heart rate from sensor
//     unsigned long starttime = millis();
//     int count = 0;
//     while (millis() < starttime + 20000) {
//       int sensorValue = analogRead(sensorPin);
//       if (sensorValue >= 590 && sensorValue <= 680) {
//         count++;
//         delay(10);
//       }
//       delay(50);
//     }
    
//     // Calculate heart rate (BPM)
//     heartrate = count * 3;


//     // Print heart rate to serial monitor
//     Serial.print("Heart rate: ");
//     Serial.println(heartrate);

// }
  smartDelay(10);

    if (millis() - lastMonitorTime >= MONITOR_INTERVAL) {
    lastMonitorTime = millis();
      if (gps.location.isValid()) {
        if (!isWithinGeofence(gps.location.lat(), gps.location.lng())) {
          if (!outOfGeofence) {
            flag=false;
            publishGeoFenceMessgage();
            outOfGeofence = true;
          }
        } 
        else {
          flag=true;
          publishGeoFenceMessgage();
          outOfGeofence = false;
        }
      }
    }

  // Check if the push button is pressed
  if (digitalRead(PUSH_BUTTON_PIN) == LOW) {
    Serial.println("Attempting to Publish location message to AWS");
    buttonPressed = true;
    Serial.println("Set button value to true");

    delay(200); // debounce delay
  } 
  else {
    if (buttonPressed) {
      if (gps.altitude.isValid()) {
        altitude = gps.altitude.meters();
      }
      if (gps.time.isValid()) {
        hr_val = gps.time.hour();     /* Get hour */
        min_val = gps.time.minute();   /* Get minutes */
        sec_val = gps.time.second();
      }
      if (gps.location.isValid()) {
        Serial.println("Location valid: Publishing location message to AWS");
        publishSosLocationMessage(gps.location.lat(), gps.location.lng(), altitude, hr_val, min_val, sec_val);
      }
      buttonPressed = false; // Reset button state
    }
  }

  if (!client.connected()) {
    connectAWS();
  } 
  else {
    client.loop();
  }
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (GPS_SoftSerial.available())
      gps.encode(GPS_SoftSerial.read());
  } while (millis() - start < ms);
}
