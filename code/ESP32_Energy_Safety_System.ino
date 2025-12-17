// ----------------------------------------------------------------------
// 1. BLYNK AUTH & LIBRARIES
// ----------------------------------------------------------------------
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6WFQbJGqE"
#define BLYNK_TEMPLATE_NAME "HomeAutomation"
#define BLYNK_AUTH_TOKEN "1vmu-O-ChkYl1Kepv0SIVhJZFlbu3L50"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi
char ssid[] = "STC_WiFi_2.4G";
char pass[] = "0505595918A";

// ----------------------------------------------------------------------
// 2. PIN DEFINITIONS
// ----------------------------------------------------------------------
const int BUZZER_PIN = 25;
const int LED1_RELAY_PIN = 26;
const int LED2_RELAY_PIN = 12;
const int FAN_RELAY_PIN = 27;

const int GAS_MQ2_PIN = 32;
const int SMOKE_MQ135_PIN = 33;

const int CURRENT_L1_PIN = 34;
const int CURRENT_L2_PIN = 35;
const int CURRENT_FAN_PIN = 15;

const int SERVO_PIN = 2;

// ----------------------------------------------------------------------
// 3. GLOBAL OBJECTS
// ----------------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo windowServo;
BlynkTimer timer;

bool isConnected = false;
bool alarmActive = false;

unsigned long startTime = 0;           
unsigned long alarmStartTime = 0;       
bool buzzerOn = false;                  

// Thresholds
const int GAS_THRESHOLD = 1000;
const int SMOKE_THRESHOLD = 300;

// ----------------------------------------------------------------------
// FUNCTIONS
// ----------------------------------------------------------------------

void displayMessage(const char* line1, const char* line2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void checkAndDisplay() {

    
    if (millis() - startTime < 20000) {
        displayMessage("Warming Up...", "");
        digitalWrite(BUZZER_PIN, LOW);
        windowServo.write(0);
        return;
    }

    
    float gasValue = analogRead(GAS_MQ2_PIN);
    float smokeValue = analogRead(SMOKE_MQ135_PIN);

    Serial.print("MQ2:"); Serial.print(gasValue);
    Serial.print(" | Smk:"); Serial.println(smokeValue);

    // ------------------------------------------------------------------
    // GAS ALERT
    // ------------------------------------------------------------------
    if (gasValue > GAS_THRESHOLD || smokeValue > SMOKE_THRESHOLD) {

        
        if (!alarmActive) {
            alarmActive = true;
            alarmStartTime = millis();  
            buzzerOn = true;
        }

        
        if (gasValue > GAS_THRESHOLD) {
            displayMessage("!GAS ROOM2!", "Ventilation ON!");
        } else {
            displayMessage("!SMOKE ROOM2!", "Ventilation ON!");
        }

        
        windowServo.write(90);

        
        if (buzzerOn && millis() - alarmStartTime < 10000) {
            digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(BUZZER_PIN, LOW);
            buzzerOn = false;
        }

        return; 
    }

   
    if (alarmActive) {
        alarmActive = false;
        buzzerOn = false;
        digitalWrite(BUZZER_PIN, LOW);
        windowServo.write(0);
    }

    
    char line1[17];
    char line2[17];

    snprintf(line1, 17, "Gas:%d Smk:%d", (int)gasValue, (int)smokeValue);
    snprintf(line2, 17, "System Normal");

    displayMessage(line1, line2);
}

// ----------------------------------------------------------------------
// BLYNK VIRTUAL PINS
// ----------------------------------------------------------------------
BLYNK_WRITE(V1) {
    int val1 = param.asInt();
    digitalWrite(LED1_RELAY_PIN, val1);
}

BLYNK_WRITE(V2) {
    int val2 = param.asInt();
    digitalWrite(LED2_RELAY_PIN, val2);
}

BLYNK_WRITE(V3) {
    int val3 = param.asInt();
    digitalWrite(FAN_RELAY_PIN, val3);
}

// ----------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED1_RELAY_PIN, OUTPUT);
    pinMode(LED2_RELAY_PIN, OUTPUT);
    pinMode(FAN_RELAY_PIN, OUTPUT);

    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED1_RELAY_PIN, HIGH);
    digitalWrite(LED2_RELAY_PIN, HIGH);
    digitalWrite(FAN_RELAY_PIN, HIGH);

    Wire.begin();
    lcd.init();
    lcd.backlight();

    displayMessage("System Booting", "");

    windowServo.setPeriodHertz(50);
    windowServo.attach(SERVO_PIN, 1000, 2000);
    windowServo.write(0);

    startTime = millis();

    WiFi.begin(ssid, pass);
    int tries = 0;

    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500);
        tries++;
        displayMessage("Connecting WiFi", "...");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
        isConnected = true;
    }

    timer.setInterval(1000L, checkAndDisplay);
}

// ----------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------
void loop() {
    if (isConnected) Blynk.run();
    timer.run();
