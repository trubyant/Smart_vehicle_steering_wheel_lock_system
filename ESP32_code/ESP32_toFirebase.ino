#include "esp_camera.h"
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
#include "ws2812.h"
#include "sd_read_write.h"
#define BUTTON_PIN  0
#include "WiFi.h"
#include "sntp.h"
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>

#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#include <Arduino.h>
#include "addons/RTDBHelper.h"
//GYRO
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
//LEDdisplay
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C ///< I2C address for the display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
//for SERVO
#define SERVO 21  //define the pwm pin
#define SERVO_CHN 0   //define the pwm channel
#define SERVO_FRQ 50  //define the pwm frequency
#define SERVO_BIT 12  //define the pwm precision
//#define buttonpin 10 //
void servo_set_pin(int pin);
void servo_set_angle(int angle);

#define PushButton1 20  //define the pwm frequency
#define PushButton2 48  //define the pwm precision

/* 1. Define the WiFi credentials */
#define WIFI_SSID1 "***********"
#define WIFI_PASSWORD1 "***********"
#define WIFI_SSID2 "***********"
#define WIFI_PASSWORD2 "***********"

//for YORKU WIFI
#define EAP_IDENTITY "***********" //if connecting from another corporation, use identity@organisation.domain in Eduroam 
#define EAP_USERNAME "***********" //oftentimes just a repeat of the identity
#define EAP_PASSWORD "***********" //your Eduroam password
const char* ssid = "AirYorkPLUS"; // Eduroam SSID

// Replace with your Firebase project credentials
#define API_KEY "***********"   //Anton's cred.
#define USER_EMAIL "***********"     
#define USER_PASSWORD "***********"      
// Firebase Storage bucket
#define STORAGE_BUCKET_ID "***********"      //Anton's cred.
#define STORAGE_BUCKET_ID_ENROLL "***********"    //Anton's Enrol
// RTDB URL
#define DATABASE_URL "***********" //Anton's cred.


FirebaseData fbdoDB; // For Realtime Database 
FirebaseData fbdoStorage; // For Storage 
FirebaseAuth auth;
FirebaseConfig config;


#define VIB_PIN 2
int thresholdVibration = 0;

// Pin number for the PIR sensor's output
#define PIR_PIN 1
#define PIEZO_PIN 19
#define LED_PIN LED_BUILTIN
#define LED_PIN1 14
#define SCL 41
#define SDA 42
#define SERVO 21
// #define VOICE 47

// GPS
static const int RXPin = 47, TXPin = 45;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// Deep Sleep Variables and OTHER
//RTC_DATA_ATTR int PIR_PIN = 1;
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool ReWakeup = false;
RTC_DATA_ATTR bool LockState = false;
bool signupOK = false;
bool AuthorizationCheck;
bool firstAuthorizationCheck = true;
bool ForceAuthorization = false;
bool LockCheck = false;
bool Measurealarmtime = true;
bool ML_end = false;
bool codePin_end = false;
bool codePin_result = false;
//bool codePingame_end = false;
bool Cognitive_end = false;
bool image_send_end = false;
unsigned long imagetime = 0;
unsigned long lastMotionTime = 0;
unsigned long lastServoMotionTime = 0;
unsigned long alarmtime = 0;
unsigned long codepintime = 0;
unsigned long CGtime = 0;
int CGfail = 0;
int Wifinumber = 0;
bool isMotionDetected = false;
bool alarmCheck = false;
bool StopAlarm = false;
bool Vibration = false;
bool ML_2 = false;
bool CognitiveGameResult = false;
bool CGreset_en = true;
int photonumber = 0;
int randNum =  0;
int codePinTrial =  1;
int currentHour;
bool enrollInit = false;
bool enrollTrigger = false;
bool enrollEnd = false;
bool imagestart = false;
//bool sleepmode = false;
RTC_DATA_ATTR gpio_num_t wake_upKey;
String enrollName;
String textfileName;


unsigned long pre_seconds = 5;

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

enum State {
  VERIFICATION,
  AUTHORIZATIONEDCISION,
  USER,
  INTRUDER,
  USERMODE,
  SLEEMODE1,
  SLEEMODE2,
  ALARM,
  COGNITIVEGAME,
  WAITCOGNITIVEGAME,
  ENROLLTRIGGER,
  ENROLL
};

RTC_DATA_ATTR State currentState = VERIFICATION;
RTC_DATA_ATTR int statesending = 9; 

String stateToString(State state) {
    switch (state) {
        case VERIFICATION: return "VERIFICATION";
        case AUTHORIZATIONEDCISION: return "AUTHORIZATIONEDCISION"; // Consider renaming to AUTHORIZATIONDECISION
        case USER: return "USER";
        case INTRUDER: return "INTRUDER";
        case USERMODE: return "USERMODE";
        case SLEEMODE1: return "SLEEMODE1"; // Consider renaming to SLEEPMODE1
        case SLEEMODE2: return "SLEEMODE2"; // Consider renaming to SLEEPMODE2
        case ALARM: return "ALARM";
        case COGNITIVEGAME: return "COGNITIVEGAME";
        case WAITCOGNITIVEGAME: return "WAITCOGNITIVEGAME";
        case ENROLLTRIGGER: return "ENROLLTRIGGER";
        case ENROLL: return "ENROLL";
        default: return "UNKNOWN STATE";
    }
}


void setup() {
	Serial.begin(115200);
  //ss.begin(GPSBaud);
	Serial.setDebugOutput(false);
  //ss.begin(GPSBaud);
	Serial.println();
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	ws2812Init();
	sdmmcInit();
	if(cameraSetup()==1){
		ws2812SetColor(2);
	}
	else{
		ws2812SetColor(1);
		return;
	}
	pinMode(PIEZO_PIN, OUTPUT);
	pinMode(PIR_PIN, INPUT);
	pinMode(LED_PIN, OUTPUT);
  pinMode(VIB_PIN, INPUT);
  pinMode(PushButton2, INPUT);
  pinMode(LED_PIN1, OUTPUT);
  
  //pinMode(buttonpin, INPUT);
	//pinMode(SERVO, OUTPUT);
	servo_set_pin(SERVO);
	//SCL, SDA
	Wire.begin(SDA, SCL);
	while (!Serial)
	  delay(10); // will pause Zero, Leonardo, etc until serial console opens
	Serial.println("time hold");
	delay(10);
  createDir(SD_MMC, "/camera");
  //listDir(SD_MMC, "/camera", 0);

	//Board wakeup
	++bootCount;
	Serial.println("Boot number: " + String(bootCount));
  Serial.println(currentState);
  /*
  if (currentState == SLEEMODE1) {
    esp_sleep_enable_ext0_wakeup(gpio_num_t(PIR_PIN), 1); // go to verification;
    Serial.println(currentState);
  } else if (currentState == SLEEMODE2) {
    esp_sleep_enable_ext0_wakeup(gpio_num_t(PIR_PIN), 1); // read button and go to usermode;
    Serial.println(currentState);
  }*/
  //esp_sleep_enable_ext0_wakeup(gpio_num_t(PIR_PIN), 1);
  //Serial.println(PIR_PIN);
  /*
  if (currentState == SLEEMODE1) {
    //wake_upKey = GPIO_NUM_1; // Vibration sensor connected to GPIO 1
    Serial.println("Waking up from SLEEMODE1 using vibration sensor (GPIO 1)");
    //currentState = SLEEMODE2;
  } else if (currentState == SLEEMODE2) {
    //wake_upKey = GPIO_NUM_2; // Motion sensor connected to GPIO 2
    Serial.println("Waking up from SLEEMODE2 using motion sensor (GPIO 2)");
    //currentState = SLEEMODE1;
  }*/
  //esp_sleep_enable_ext0_wakeup(wake_upKey, 1);
  esp_sleep_enable_ext0_wakeup(gpio_num_t(PIR_PIN), 1);
  if (bootCount == 1) { // adjust servo
    for (int i = 45; i > 0; i--) {  //make light fade out
          servo_set_angle(i);
          delay(10);
          Serial.println("Lock");
    }
  }
  // Initialize Wi-Fi
	WiFi.begin(WIFI_SSID1, WIFI_PASSWORD1);
	Serial.print("Connecting to Wi-Fi");
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
    Serial.println(Wifinumber);
    ++Wifinumber;
		delay(300);
    if (Wifinumber == 10){
      WiFi.begin(WIFI_SSID2, WIFI_PASSWORD2);
    } else if (Wifinumber == 30){
      WiFi.begin(WIFI_SSID1, WIFI_PASSWORD1);
      Wifinumber = 0;
    } else if (Wifinumber == 20) {
      WiFi.mode(WIFI_STA); 
      WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
      //Wifinumber = 0;
    }
	}
	Serial.println();
  if (Wifinumber >= 10 && Wifinumber < 20) {
    Serial.print("Connected with WIFI: ");
    Serial.println(WIFI_SSID2);
  } else if (Wifinumber >= 20 && Wifinumber < 30) {
    Serial.print("Connected with WIFI: ");
    Serial.println(ssid);
  } else {
    Serial.print("Connected with WIFI: ");
    Serial.println(WIFI_SSID1);
  }
	Serial.print("Connected with IP: ");
	Serial.println(WiFi.localIP());
	Serial.println();
  ss.begin(GPSBaud);
	// Setup Firebase configuration
	config.api_key = API_KEY;
	auth.user.email = USER_EMAIL;
	auth.user.password = USER_PASSWORD;
	config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
	// Initialize Firebase
	Firebase.begin(&config, &auth);
	Firebase.reconnectWiFi(true); // Handle WiFi reconnections automatically

	// Wait for Firebase to be ready with a simple blocking delay (not recommended for production code)
  if (bootCount%10 == 0) {
    removeDir(SD_MMC, "/camera");
    createDir(SD_MMC, "/camera");
  }
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

	//OTHER
	isMotionDetected = digitalRead(PIR_PIN) == HIGH;
	if (isMotionDetected) {
		lastMotionTime = millis();
		Serial.println("Motion detected at startup");
		digitalWrite(LED_PIN, HIGH);
		analogWrite(PIEZO_PIN, 0);
		if (Firebase.ready() && signupOK) {
			Firebase.RTDB.setString(&fbdoDB, "/motion", "ON");
      Serial.println("Realtime /motion is set to ON");
		}
	} else {
		lastMotionTime = 0;
	}

  sntp_set_time_sync_notification_cb( timeavailable );
  sntp_servermode_dhcp(1);    // (optional)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
	//LEDdisplay
	// Manually specify the SDA and SCL pins
	Wire.begin(42, 41); // SDA, SCL

	// Initialize OLED display
	if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.println(F("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever
	}

  // 5. SLEEMODE1 state
  if (currentState == SLEEMODE1) {
    //lcd_display (1, SSD1306_WHITE, 0, 0, "Hello, Please wait booting MCU to get Verification");
    Serial.println("GO TO VERIFICATION STATE");
    Serial.println("Hello, Please wait booting MCU to get Verification");
    currentState = VERIFICATION; // go to Verification
  } 
  // 6. SLEEMODE2 state
  /*else if (currentState == SLEEMODE2) {
    lcd_display (1, SSD1306_WHITE, 0, 0, "Hello User, Please wait booting MCU to control Lock");
    Serial.println("GO TO USERMODE STATE");
    Serial.println("Hello User, Please wait booting MCU to control Lock");
    currentState = USERMODE; // go to UserMODe
  }*/
  photonumber = 0;
  randNum = random(100000,999999);
  
  // reset variables
  /*if (bootCount == 1) {
    lcd_display (1, SSD1306_WHITE, 0, 0, "Reset Everything");
    delay(50);
    Firebase.RTDB.setBool(&fbdoDB, "/LockHandler", false);
    Firebase.RTDB.setBool(&fbdoDB, "/ForceAuthorization", false);
    Firebase.RTDB.setBool(&fbdoDB, "/Authorization", false);
    Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameResult", false);
    Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", false);
    Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGame_end", false);
    Firebase.RTDB.setInt(&fbdoDB, "/codePin", 0);
    Firebase.RTDB.setBool(&fbdoDB, "/codePin_result", false);
    Firebase.RTDB.setBool(&fbdoDB, "/codePingame_end", false);
    //Firebase.RTDB.setBool(&fbdoDB, "/enrollEnd", false);
    //Firebase.RTDB.setBool(&fbdoDB, "/enrollInit", false);
    Firebase.RTDB.setBool(&fbdoDB, "/servoControl", false);
  }*/
  /*for (int i = 45; i > 0; i--) {  //make light fade out
        servo_set_angle(i);
        delay(10);
        Serial.println("Lock");
  }*/
  delay(50);
  imagetime = millis();
  lcd_display (1, SSD1306_WHITE, 0, 0, "Press the button within 10s to take picture");
  Serial.println("Press the button within 10s to take picture");
}


// ============================================= Loop Start ======================================================
void loop() {
  bool currentMotion = digitalRead(PIR_PIN) == HIGH;

  // Sending state
  if (statesending != (int)currentState) {
    statesending = (int)currentState;
    String currentStateString = stateToString(currentState);
    Firebase.RTDB.setString(&fbdoDB, "/currentstate", currentStateString);   
  }
  

  // ================================ 0. VERIFICATION state ================================
  if (currentState == VERIFICATION) {

    
    if (digitalRead(PushButton2) == HIGH && photonumber == 0) {
      Serial.println("Start to take pictures and codePin access");
      imagestart = true;
    }


    if (imagestart && photonumber < 2) {
      digitalWrite(LED_PIN1, HIGH);
      send_picture_storage(photonumber);
      digitalWrite(LED_PIN1, LOW);
      ++photonumber;
      imagetime = millis();
    }

    if (Firebase.ready() && signupOK) {
      if (Firebase.RTDB.getBool(&fbdoDB, "/ML_end")) {
        ML_end = fbdoDB.boolData();
      }
      delay(70);

      if (Firebase.RTDB.getBool(&fbdoDB, "/enrollTrigger")) {
        enrollTrigger = fbdoDB.boolData();
      delay(100);
      }
    }

    if (Firebase.RTDB.getBool(&fbdoDB, "/ForceAuthorization")) {
      ForceAuthorization = fbdoDB.boolData();
      if (ForceAuthorization) {
        Serial.println("ForceAuthorization is deteced");
        Firebase.RTDB.setBool(&fbdoDB, "/ForceAuthorization", false);
      }
      delay(50);
    }


    int vib_sensor = analogRead(VIB_PIN);
    delay(10);
    if(thresholdVibration >= 5)  {
      Vibration = true;
      Serial.println("ALERT!!!");
      delay(10);
    } else if (vib_sensor/1000 >= 1)  {
      Serial.println(vib_sensor);
      thresholdVibration++;
      delay(10);
    }

    //if (ML_end && codePin_end) {
    if (ML_end) { 
      Serial.println("GO TO CHECKAUTHORIZATION STATE");
      currentState = AUTHORIZATIONEDCISION;
      photonumber = 0;
      String accesss_code = "Access Code: " + String(randNum);
      lcd_display (1, SSD1306_WHITE, 0, 0, accesss_code.c_str());
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", randNum);
      delay(100);
    } //else if (Vibration || (!imagestart && (millis() - imagetime) > 20000)) {
      else if (Vibration || (millis() - imagetime) > 20000) {
      Serial.println("GO TO INTRUDER STATE");
      photonumber = 0;
      currentState = INTRUDER;
      Vibration = false;
      imagestart = false;
      delay(100);
    } else if (ForceAuthorization) {
      lcd_display (1, SSD1306_WHITE, 0, 0, "ForceAuthorization Detected, Go to USER STATE");
      Serial.println("GO TO USER STATE");
      AuthorizationCheck = true;
      firstAuthorizationCheck = false;
      currentState = USER;
      photonumber = 0;
      delay(100);
    } else if (enrollTrigger) {
      Serial.println("GO TO ENROLLTRIGGER STATE");
      lcd_display (1, SSD1306_WHITE, 0, 0, "Fill out ID, Password to enroll");
      currentState = ENROLLTRIGGER;
      photonumber = 0;
      delay(50); 
    }
  } 

  // ================================ 1. AUTHORIZATIONEDCISION state ================================
  else if (currentState == AUTHORIZATIONEDCISION) {
    if (Firebase.ready() && signupOK) {
      if (firstAuthorizationCheck) {
        if (Firebase.RTDB.getBool(&fbdoDB, "/Authorization")){
          AuthorizationCheck = fbdoDB.boolData();
        }
        delay(50);
        firstAuthorizationCheck = false;
        codepintime = millis();
      }
      if (Firebase.RTDB.getBool(&fbdoDB, "/ML_2")) {
        ML_2 = fbdoDB.boolData();
        delay(50);
      }

      if (Firebase.RTDB.getBool(&fbdoDB, "/codePin_end")){
        codePin_end = fbdoDB.boolData();
        delay(50);
      }

      if (CGreset_en) {
        Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", false);
        delay(50);
        CGreset_en = false;
      }
    }

    if (codePin_end) {
      if (Firebase.RTDB.getBool(&fbdoDB, "/codePin_result")){
        codePin_result = fbdoDB.boolData();
        delay(100);
      }
    }

    if (Firebase.RTDB.getBool(&fbdoDB, "/ForceAuthorization")) {
      ForceAuthorization = fbdoDB.boolData();
      if (ForceAuthorization) {
        Serial.println("ForceAuthorization is deteced");
        Firebase.RTDB.setBool(&fbdoDB, "/ForceAuthorization", false);
      }
      delay(50);
    }

    
    if ((AuthorizationCheck && codePin_result)|| ForceAuthorization) {
      Serial.println("GO TO USER STATE");
      currentState = USER;
      AuthorizationCheck = true;
      firstAuthorizationCheck = false;
      //Firebase.RTDB.setInt(&fbdoDB, "/codePin", 0);
      if (Firebase.ready() && signupOK) {
        if (ML_2) {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", false);
        } else {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", true);
        }
        //Firebase.RTDB.setBool(&fbdoDB, "/ML_end", false); // test 
        //Firebase.RTDB.setBool(&fbdoDB, "/codePin_end", false); // test 
      }
      lcd_display (1, SSD1306_WHITE, 0, 0, "User detected");
      delay(100);
    } else if (AuthorizationCheck && !codePin_result && codePin_end){
      currentState = AUTHORIZATIONEDCISION;
      if (Firebase.ready() && signupOK) {
        if (ML_2) {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", false);
        } else {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", true);
        }
        //Firebase.RTDB.setBool(&fbdoDB, "/ML_end", false); // test 
        //Firebase.RTDB.setBool(&fbdoDB, "/codePin_end", false); // test 
        //codePinTrial++;
      }
      codePinTrial++;
      lcd_display (1, SSD1306_WHITE, 0, 0, "User detected, but failed code pin");
      delay(100); 
    } else if (!AuthorizationCheck) {
      Serial.println("GO TO INTRUDER STATE");
      currentState = INTRUDER;
      if (Firebase.ready() && signupOK) {
        if (ML_2) {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", false);
        } else {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", true);
        }
        //Firebase.RTDB.setBool(&fbdoDB, "/ML_end", false); // test 
        //Firebase.RTDB.setBool(&fbdoDB, "/codePin_end", false); // test 
      }
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", 0);
      lcd_display (1, SSD1306_WHITE, 0, 0, "INTRUDER detected"); 
      delay(100);
    } else if (AuthorizationCheck && (millis() - codepintime > 30000 || codePinTrial > 3)) {
      if (Firebase.ready() && signupOK) {
        Firebase.RTDB.setBool(&fbdoDB, "/codePinTrial", true);
        if (ML_2) {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", false);
        } else {
          Firebase.RTDB.setBool(&fbdoDB, "/ML_2", true);
        }
        //Firebase.RTDB.setBool(&fbdoDB, "/ML_end", false); // test 
        //Firebase.RTDB.setBool(&fbdoDB, "/codePin_end", false); // test 
      }
      codepintime = millis();
      codePinTrial = 1;
      currentState = AUTHORIZATIONEDCISION;
      //firstAuthorizationCheck = true;
      //lcd_display (1, SSD1306_WHITE, 0, 0, "User detected, but failed code pin 3 times");
      randNum = random(100000,999999);
      String accesss_code = "Failed 3 time New Code: " + String(randNum);
      lcd_display (1, SSD1306_WHITE, 0, 0, accesss_code.c_str());
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", randNum);
      delay(100);
      Firebase.RTDB.setBool(&fbdoDB, "/codePinTrial", false);
    }
  } 

  // ================================ 2. USER state ================================
  else if (currentState == USER) {
    Serial.println("currentState is User");

    struct tm timeinfo;
    int currentHour = timeinfo.tm_hour;
    delay(50);
    Serial.println(currentHour);
    
    if (currentHour < 14 && currentHour >= 3) {
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", 0);
      digitalWrite(LED_PIN1, HIGH);
      Firebase.RTDB.setBool(&fbdoDB, "/LockHandler", true);
      for (int i = 0; i < 45; i++) {  //make light fade in
            servo_set_angle(i);
            delay(10);
            Serial.println("UnLock");
      }
      delay(50);
      Firebase.RTDB.setBool(&fbdoDB, "/servoControl", true);
      delay(50);
      ForceAuthorization = false;
      LockState = true;
      lastServoMotionTime = millis();
      lcd_display (1, SSD1306_WHITE, 0, 0, "USERMODE, Control Servo with App!");
      Serial.println("GO TO USERMODE STATE");
      currentState = USERMODE;
      delay(100);
    } else {
      Firebase.RTDB.setBool(&fbdoDB, "/timeCurrent", true);
      currentState = COGNITIVEGAME;
      lcd_display (1, SSD1306_WHITE, 0, 0, "Start Cognitive Game");
      Serial.println("GO TO COGNITIVEGAME STATE");
      delay(100);
    }
  } 

  // ================================ 3. INTRUDER state ================================
  else if (currentState == INTRUDER) {
    Serial.println("INTRUDER Detected");
    lcd_display (1, SSD1306_WHITE, 0, 0, "INTRUDER Detected");
    Serial.println("GO TO ALARM STATE");

    
    currentState = ALARM;
  }

  // ================================ 4. USERMODE state ================================
  else if (currentState == USERMODE) {

    if (Firebase.RTDB.getBool(&fbdoDB, "/servoControl")) {
        LockCheck = fbdoDB.boolData();
        delay(50);
    }

    if (CGreset_en) {
      Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", false);
      delay(50);
      CGreset_en = false;
    }

    // Through APP  
    if (LockCheck && !LockState) {
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Unlock");
      for (int i = 0; i < 45; i++) {  //make light fade in
        servo_set_angle(i);
        delay(10);
        Serial.println("UnLock");
      }
      lastServoMotionTime = millis();
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Unlock");
      LockState = true;
    } else if (!LockCheck && LockState) {
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Lock");
      for (int i = 45; i > 0; i--) {  //make light fade out
        servo_set_angle(i);
        delay(10);
        Serial.println("Lock");
      }
      lastServoMotionTime = millis();
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Lock");
      LockState = false;
    }

    if ((millis() - lastServoMotionTime) > 20000 && !LockState) { // when go to sleep with lock
      digitalWrite(LED_PIN1, LOW);
      Serial.println("GO TO SLEEMODE1 STATE");
      Serial.println("Lock the servo befre sleep mode");
      currentState = SLEEMODE1; // go to verfication after wake up
      Serial.println(currentState);
      Serial.println("Go to sleep mode with Lock, VERIFICATION after wake up");
      Firebase.RTDB.setBool(&fbdoDB, "/LockHandler", false);
      lcd_display (1, SSD1306_WHITE, 0, 0, "");
      display.clearDisplay();
      delay(5000);
      esp_deep_sleep_start();
    } else if (millis() - lastServoMotionTime > 20000 && LockState) { // when go to sleep with unlcok
      /*Serial.println("GO TO SLEEMODE2 STATE");
      ReWakeup = true;
      currentState = SLEEMODE2; // go to user mode after wake up
      //PIR_PIN = 48;
      //#undef PIR_PIN
      //#define PIR_PIN 48
      //pinMode(PIR_PIN, INPUT);
      //Serial.println(PIR_PIN);
      Serial.println(currentState);
      Serial.println("Go to sleep mode with Unlock, USERMODE after wake up"); */
      digitalWrite(LED_PIN1, LOW);
      for (int i = 45; i > 0; i--) {  //make it lock before when it unlock
        servo_set_angle(i);
        delay(10);
        Serial.println("Lock");
      }
      Firebase.RTDB.setBool(&fbdoDB, "/LockHandler", false);
      Firebase.RTDB.setBool(&fbdoDB, "/servoControl", false);
      LockState = false;
      delay(50);
      Serial.println("GO TO SLEEMODE1 STATE");
      currentState = SLEEMODE1;
      Serial.println(currentState);
      Serial.println("Go to sleep mode with Lock, VERIFICATION after wake up");
      lcd_display (1, SSD1306_WHITE, 0, 0, "");
      display.clearDisplay();
      delay(5000);
      esp_deep_sleep_start();
    }

  }

  // ================================ 7. ALARM state ================================
  else if (currentState == ALARM) {
    
    if (Measurealarmtime && !StopAlarm) {
      alarmtime = millis();
      Measurealarmtime = false;
      alarmCheck = true;
    }

    if ((millis()-alarmtime) > 500 && !Measurealarmtime){
      alarmCheck = false;
      if ((millis()-alarmtime) > 1000) {
        Measurealarmtime = true;
      }
    }

    if (Firebase.ready() && signupOK) {
      if (Firebase.RTDB.getBool(&fbdoDB, "/alarm")) {
        StopAlarm = fbdoDB.boolData();
        delay(100);
      }
	  }

    if (Firebase.RTDB.getBool(&fbdoDB, "/ForceAuthorization")) {
      ForceAuthorization = fbdoDB.boolData();
      if (ForceAuthorization) {
        Serial.println("ForceAuthorization is deteced");
        //lcd_display (1, SSD1306_WHITE, 0, 0, "ForceAuthorization Detected");
        Firebase.RTDB.setBool(&fbdoDB, "/ForceAuthorization", false);
      }
      delay(70);
    }

    analogWrite(PIEZO_PIN, (alarmCheck && !StopAlarm) ? 50 : 0);
    if (photonumber < 5 && (millis() - imagetime) > 10000) {
      imagetime = millis();
      store_image_SD(photonumber);
      //Serial.println(photonumber);
      ++photonumber;
    }
    
    if (ForceAuthorization) {
      StopAlarm = true;
      analogWrite(PIEZO_PIN, (alarmCheck && !StopAlarm) ? 50 : 0);
      Serial.println("GO TO USER STATE");
      lcd_display (1, SSD1306_WHITE, 0, 0, "ForceAuthorization Detected, USERSTATE");
      currentState = USER;
      photonumber = 0;
    }
  }

  // ================================ 8. COGNITIVEGAME state ================================
  else if (currentState == COGNITIVEGAME) {

    if (Firebase.RTDB.getBool(&fbdoDB, "/CognitiveGame_end")){
        Cognitive_end  = fbdoDB.boolData();
        delay(50);
    }
    

    if  (Cognitive_end) {
      if (Firebase.RTDB.getBool(&fbdoDB, "/CognitiveGameResult")){
        CognitiveGameResult = fbdoDB.boolData();
        delay(50);
      }
    }

    if (CognitiveGameResult && Cognitive_end) {
      //lcd_display (1, SSD1306_WHITE, 0, 0, "UnLock");
      Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", true);
      digitalWrite(LED_PIN1, HIGH);
      CognitiveGameResult = false;
      Serial.println("GO TO USER STATE");
      Firebase.RTDB.setBool(&fbdoDB, "/LockHandler", true);
      for (int i = 0; i < 45; i++) {  //make light fade in
        servo_set_angle(i);
        delay(10);
        Serial.println("UnLock");
      }
      delay(50);
      lcd_display (1, SSD1306_WHITE, 0, 0, "USERMODE, Control Servo with App!");
      Firebase.RTDB.setBool(&fbdoDB, "/servoControl", true);
      delay(50);
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", 0);
      ForceAuthorization = false;
      LockState = true;
      lastServoMotionTime = millis();
      Serial.println("GO TO USERMODE STATE");
      currentState = USERMODE;
      CGreset_en = true;
      delay(100);
    } else if (!CognitiveGameResult && Cognitive_end) {
      Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", true);
      lcd_display (1, SSD1306_WHITE, 0, 0, "Fail, WAIT COGNITIVEGAME");
      CGreset_en = true;
      currentState = WAITCOGNITIVEGAME;
      ++CGfail;
      delay(100);
      Serial.println("GO TO WAITCOGNITIVEGAME STATE");
    }
    //Cognitive_end = false;
    delay(50);
  }
  // ================================ 9. WAITCOGNITIVEGAME state ================================
  else if (currentState == WAITCOGNITIVEGAME) {
    if (CGreset_en) {
      Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", false);
      delay(50);
      CGreset_en = false;
      CGtime = millis();
      Cognitive_end = false;
    }
    
    if (millis() - CGtime > 30000 && CGfail == 1) {
      currentState = AUTHORIZATIONEDCISION;
      //Firebase.RTDB.setBool(&fbdoDB, "/CognitiveGameReset", true);
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Fail Cognitive 3 times Re-Verification need");
      currentState = AUTHORIZATIONEDCISION;
      Serial.println("GO TO AUTHORIZATIONEDCISION STATE");
      delay(100);
      randNum = random(100000,999999);
      String accesss_code = "Failed 3 time New Code: " + String(randNum);
      lcd_display (1, SSD1306_WHITE, 0, 0, accesss_code.c_str());
      Firebase.RTDB.setInt(&fbdoDB, "/codePin", randNum);
      CGfail = 0;
    } /*else if (millis() - CGtime > 20000 && CGfail == 2) {
      currentState = COGNITIVEGAME;
      Serial.println("GO TO COGNITIVEGAME STATE");
      delay(100);
    } else if (millis() - CGtime > 30000 && CGfail == 3) {
      currentState = COGNITIVEGAME;
      Serial.println("GO TO COGNITIVEGAME STATE");
      delay(100);
    }*/
  }

  // ================================ 10. ENROLLTRIGGER state ================================
  else if (currentState == ENROLLTRIGGER) {
    
    if (Firebase.RTDB.getBool(&fbdoDB, "/enrollInit")) {
        enrollInit = fbdoDB.boolData();
        delay(50);
    }

    if (Firebase.RTDB.getBool(&fbdoDB, "/enrollTrigger")) {
      enrollTrigger = fbdoDB.boolData();
      delay(50);
    }


    if (enrollInit) {
      Serial.println("GO TO ENROLL STATE");
      currentState = ENROLL;
      enrollInit = false;
      Firebase.RTDB.setBool(&fbdoDB, "/enrollInit", false);
      delay(50); 
      lcd_display (1, SSD1306_WHITE, 0, 0, "Press the button to take picture");
    } else if (!enrollTrigger) {
      currentState = VERIFICATION;
      imagetime = millis();
      delay(50); 
    }
  }

  // ================================ 11. ENROLL state ================================
  else if (currentState == ENROLL) {


    if (digitalRead(PushButton2) == HIGH && photonumber == 0) {
      Serial.println("taking picture to enroll User");
      //lcd_display (1, SSD1306_WHITE, 0, 0, "Wait 5 senconds to take picture and to enroll User");
      //delay(5000);
      imagestart = true;
      if (Firebase.RTDB.getString(&fbdoDB, "/enrollName")){
        enrollName = fbdoDB.stringData();
        textfileName = "/" + enrollName + "_DONE.txt";
        delay(50);
      }
    }

    if (imagestart && photonumber < 31) {
      digitalWrite(LED_PIN1, HIGH);
      send_picture_storage_enrol(photonumber, enrollName.c_str());
      digitalWrite(LED_PIN1, LOW);
      //Serial.println(photonumber);
      ++photonumber;
    } else if (photonumber == 31) {
      //writeFile(SD_MMC, textfileName.c_str(), "dd");
      send_textfile(textfileName.c_str());
      Firebase.RTDB.setBool(&fbdoDB, "/enrollEnd", true);
      enrollEnd = true;
    }

    if (enrollEnd) {
      lcd_display (1, SSD1306_WHITE, 0, 0, "Press the button within 10s to take picture");
      Serial.println("GO TO VERIFICATION STATE");
      enrollEnd = false;
      currentState = VERIFICATION;
      imagestart = false;
      photonumber = 0;
      delay(40000);
      imagetime = millis();
    }
  }

 
  
  // ================================ Force authorization Enable ================================
  /*if (Firebase.RTDB.getBool(&fbdoDB, "/ForceAuthorization")) {
    ForceAuthorization = fbdoDB.boolData();
    if (ForceAuthorization) {
      Serial.println("ForceAuthorization is deteced");
      lcd_display (1, SSD1306_WHITE, 0, 0, "ForceAuthorization Detected");
      Firebase.RTDB.setBool(&fbdoDB, "/ForceAuthorization", false);
    }
    delay(50);
  }
  */  
  while (ss.available() > 0)
    if (gps.encode(ss.read()))
    //  displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    //Serial.println(F("No GPS detected: check wiring."));
    //while(true);
  }

  yield();
  delay(100);
}


int cameraSetup(void) {
	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	//config.frame_size = FRAMESIZE_UXGA;
	config.frame_size = FRAMESIZE_SVGA; // 800 x 600
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
	config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
	config.fb_location = CAMERA_FB_IN_PSRAM;
	config.jpeg_quality = 12;
	config.fb_count = 1;

	// if PSRAM IC present, init with UXGA resolution and higher JPEG quality
	// for larger pre-allocated frame buffer.
	if(psramFound()){
		config.jpeg_quality = 10;
		config.fb_count = 2;
		config.grab_mode = CAMERA_GRAB_LATEST;
	} else {
		// Limit the frame size when PSRAM is not available
		config.frame_size = FRAMESIZE_SVGA;
		config.fb_location = CAMERA_FB_IN_DRAM;
	}

	// camera init
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed with error 0x%x", err);
		return 0;
	}

	sensor_t * s = esp_camera_sensor_get();
	// initial sensors are flipped vertically and colors are a bit saturated
	s->set_vflip(s, 1); // flip it back
	s->set_brightness(s, 1); // up the brightness just a bit
	s->set_saturation(s, 0); // lower the saturation

	Serial.println("Camera configuration complete!");
	return 1;
}


//OTHER
void servo_set_pin(int pin) {
	ledcSetup(SERVO_CHN, SERVO_FRQ, SERVO_BIT);
	ledcAttachPin(pin, SERVO_CHN);
}

void servo_set_angle(int angle) {
	if (angle > 180 || angle < 0)
		return;
	long pwm_value = map(angle, 0, 180, 103, 512);
	ledcWrite(SERVO_CHN, pwm_value);
}

void lcd_display(int textsize, int Color, int left, int top, const char* text) {
  display.clearDisplay();
	display.setTextSize(textsize);             // Normal 1:1 pixel scale
	display.setTextColor(Color); // Draw white text
	display.setCursor(left, top);              // Start at top-left corner
  display.setRotation(2);
	display.print(text);
	display.display();    
}

void send_picture_storage(int photo_index) {
  //int photo_index = 0; 
  //for (int i = 0; i < 4; i++) {
				// Capture an image
				camera_fb_t *fb = esp_camera_fb_get();
				if (fb != NULL) {
					// Generate the filename based on the index
					//String fileName = "user_pic_" + String(photo_index) + ".jpg";
          String fileName = "user_pic_" + String(photo_index) + ".png";
					//String imagePath = "/camera/" + fileName;

					if (photo_index != 0) { // Skip uploading the first image (user_pic_0.jpg)
						// Check if Firebase is ready for upload
						if (Firebase.ready()) {
							Serial.print("Uploading " + fileName + " to Firebase Storage... ");

							//String storagePath = "/Anton/" + fileName; //fileName; //"/images/" + fileName
              String storagePath = fileName;
							//if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID, fb->buf, fb->len, storagePath.c_str(), "image/jpeg")) {
							if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID, fb->buf, fb->len, storagePath.c_str(), "image/png")) {
              	Serial.println("Upload successful.");
								Serial.println("Download URL: " + fbdoStorage.downloadURL());
							} else {
								Serial.println("Upload failed: " + fbdoStorage.errorReason());
							}
						}
					}
					esp_camera_fb_return(fb);

					//photo_index++; // Increment photo index
					delay(100); // Add a delay to avoid taking images too quickly
				}

				// Check if we have taken 11 images, and if so, break the loop
				//if (photo_index == 5) {
				//	break;
				//}
	//}
}

void send_picture_storage_enrol(int photo_index_num, const char* name) {
  //int photo_index = 0; 
  //for (int i = 0; i < 4; i++) {
				// Capture an image
				camera_fb_t *fb = esp_camera_fb_get();
				if (fb != NULL) {
					// Generate the filename based on the index
					//String fileName = name + String(photo_index_num) + ".jpg";
          String fileName = name + String(photo_index_num) + ".png";
					//String imagePath = "/camera/" + fileName;

					if (photo_index_num != 0) { // Skip uploading the first image (user_pic_0.jpg)
						// Check if Firebase is ready for upload
						if (Firebase.ready()) {
							Serial.print("Uploading " + fileName + " to Firebase Storage... ");

							String storagePath = "/" + String(name) + "/"  + fileName; //fileName; //"/images/" + fileName

							//if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID_ENROLL, fb->buf, fb->len, storagePath.c_str(), "image/jpeg")) {
							if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID_ENROLL, fb->buf, fb->len, storagePath.c_str(), "image/png")) {
              	Serial.println("Upload successful.");
								Serial.println("Download URL: " + fbdoStorage.downloadURL());
							} else {
								Serial.println("Upload failed: " + fbdoStorage.errorReason());
							}
						}
					}
					esp_camera_fb_return(fb);

					//photo_index++; // Increment photo index
					delay(100); // Add a delay to avoid taking images too quickly
				}

				// Check if we have taken 11 images, and if so, break the loop
				//if (photo_index == 5) {
				//	break;
				//}
	//}
}


void send_textfile(const char* name) {
  
  FileData fileData = readFile(SD_MMC, name);
  String fileName = String(name);
  if (Firebase.ready()) {
		Serial.print("Uploading " + fileName + " to Firebase Storage... ");
    String storagePath = fileName; //fileName; //"/images/" + fileName
    if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID_ENROLL, fileData.buffer, fileData.size, storagePath.c_str(), "text/plain")) {
			Serial.println("text file upload successful.");
			Serial.println("Download URL: " + fbdoStorage.downloadURL());
		} else {
			Serial.println("Upload failed: " + fbdoStorage.errorReason());
			}
	}
  free(fileData.buffer);
	delay(100);

}

void store_image_SD (int photo_number) {
  camera_fb_t * fb = esp_camera_fb_get();
    if (fb != NULL) {
      photo_number = readFileNum(SD_MMC, "/camera");
      String fileName = "user_pic_" + String(photo_number) + ".png";
      if (photo_number!=-1) {
        //String path = "/camera/" + String(photo_number) +".jpg";
        String path = "/camera/" + String(photo_number) +".png";
        writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
        if (photo_number != 0 && photo_number < 3) {
          if (Firebase.ready()) {
							Serial.print("Uploading " + fileName + " to Firebase Storage... ");

							//String storagePath = "/Anton/" + fileName; //fileName; //"/images/" + fileName
              String storagePath = fileName;
							//if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID, fb->buf, fb->len, storagePath.c_str(), "image/jpeg")) {
							if (Firebase.Storage.upload(&fbdoStorage, STORAGE_BUCKET_ID, fb->buf, fb->len, storagePath.c_str(), "image/png")) {
              	Serial.println("Upload successful.");
								Serial.println("Download URL: " + fbdoStorage.downloadURL());
							} else {
								Serial.println("Upload failed: " + fbdoStorage.errorReason());
							}
						}
        }
      }
      esp_camera_fb_return(fb);
    }
}

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // int DayofWeek = timeinfo.tm_wday;
  // int Month = timeinfo.tm_mon;
  // int DateofMonth = timeinfo.tm_mday;
  // int Year = timeinfo.tm_year;
  currentHour = timeinfo.tm_hour;
  // int Min = timeinfo.tm_min;
  // int Sec = timeinfo.tm_sec;
  // Serial.println(DayofWeek);
  // Serial.println(Month);
  // Serial.println(DateofMonth);
  // Serial.println(Year);
  // Serial.println(Hour);
  // Serial.println(Min);
  // Serial.println(Sec);
  String currentTime = String(currentHour);
  Serial.println(currentTime);
}

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void displayInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    Firebase.RTDB.setDouble(&fbdoDB, "/gpsData/Latitude", gps.location.lat());
    delay(50);
    Firebase.RTDB.setDouble(&fbdoDB, "/gpsData/Longitude", gps.location.lng());
  }
  else
  {
    Serial.print(F("INVALID"));
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

