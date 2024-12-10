#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
/*
   This sample sketch demonstrates the normal use of a TinyGPSPlus (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const int RXPin = 1, TXPin = 2;
static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);


#define LED 19
#define API_KEY "********************"
#define USER_EMAIL "********************"     
#define USER_PASSWORD "********************"      
#define DATABASE_URL "https://********************" 


//for YORKU WIFI
#define EAP_IDENTITY "********************" //if connecting from another corporation, use identity@organisation.domain in Eduroam 
#define EAP_USERNAME "********************" //oftentimes just a repeat of the identity
#define EAP_PASSWORD "********************" //your Eduroam password
const char* ssid = "AirYorkPLUS"; // Eduroam SSID

//Firebase
#include "WiFi.h"
#include "sntp.h"
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
#include <Arduino.h>
#include "addons/RTDBHelper.h"
#include <Wire.h>

FirebaseData fbdoDB; // For Realtime Database 
FirebaseData fbdoStorage; // For Storage 
FirebaseAuth auth;
FirebaseConfig config;

int Wifinumber = 0;
bool signupOK = false;

void setup()
{
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPSPlus with an attached GPS module"));
  Serial.print(F("Testing TinyGPSPlus library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();

  //new

  pinMode(LED, OUTPUT);
    // Initialize Wi-Fi
	Serial.print("Connecting to Wi-Fi");
  //WiFi.begin(WIFI_SSID1, WIFI_PASSWORD1);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
    
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
    Serial.println(Wifinumber);
    ++Wifinumber;
		delay(300);  
    //WiFi.begin(WIFI_SSID1, WIFI_PASSWORD1);
	}
  Wifinumber = 0;
  Serial.print("Connected with IP: ");
	Serial.println(WiFi.localIP());
	Serial.println();

	// Setup Firebase configuration
	config.api_key = API_KEY;
	//auth.user.email = USER_EMAIL;
	//auth.user.password = USER_PASSWORD;
	config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
	// Initialize Firebase
	Firebase.begin(&config, &auth);
	Firebase.reconnectWiFi(true); // Handle WiFi reconnections automatically

	delay(5000); // Adjust delay as necessary for your application's needs

	if (Firebase.ready()) {
		Serial.println("Firebase is ready.");
	} else {
		Serial.println("Firebase is not ready after waiting.");
	}

	// Attempt to sign up or log in
	if (Firebase.signUp(&config, &auth, "", "")) {
		Serial.println("Firebase sign-up OK");
    signupOK = true;
	} else {
		Serial.printf("Firebase sign-up FAILED: %s\n", config.signer.signupError.message.c_str());
	}
  Serial.println();
}

void loop()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    digitalWrite(LED, HIGH);

    // SEND TO FIREBASE REALTIME
    Firebase.RTDB.setDouble(&fbdoDB, "/gpsData/Latitude", gps.location.lat());
    delay(50);
    Firebase.RTDB.setDouble(&fbdoDB, "/gpsData/Longitude", gps.location.lng());

  }
  else
  {
    Serial.print(F("INVALID"));
    digitalWrite(LED, LOW);
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
