#include <Keypad.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi
const char* ssid = "lily";
const char* password = "Sramli18@";

// Mobius
const char* mobiusIP = "172.20.10.6";
const int mobiusPort = 7579;

// Password
const String CORRECT_PASSWORD = "5555";
String passwordInput = "";
int failCount = 0;

// Pins
const int PIN_BUZZER = 12;
const int PIN_SERVO  = 13;
Servo myservo;

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {4, 22, 21, 19};
byte colPins[COLS] = {18, 5, 14, 27};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void sendToMobius(String status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }
  HTTPClient http;
  String url = "http://" + String(mobiusIP) + ":" + String(mobiusPort) + "/Mobius/SmartLock";
  http.begin(url);
  http.addHeader("Content-Type", "application/json;ty=4");
  http.addHeader("X-M2M-Origin", "SmartLockAE");
  http.addHeader("X-M2M-RI", "req001");
  http.addHeader("X-M2M-RVI", "2a");
  String payload = "{\"m2m:cin\":{\"con\":\"" + status + "\"}}";
  int code = http.POST(payload);
  Serial.println("Mobius response: " + String(code));
  http.end();
}

void lockout() {
  Serial.println("LOCKOUT! Too many failed attempts.");
  Serial.println("System locked for 30 seconds...");
  sendToMobius("LOCKOUT");
  
  // Buzzer panjang tanda lockout
  digitalWrite(PIN_BUZZER, HIGH);
  delay(1000);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Countdown display + flush keypad
  for (int i = 30; i > 0; i--) {
    Serial.println("Lockout: " + String(i) + "s remaining...");
    unsigned long start = millis();
    while (millis() - start < 1000) {
      customKeypad.getKey(); // flush semua keypress
    }
  }
  
  Serial.println("System unlocked. Try again.");
  failCount = 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  myservo.attach(PIN_SERVO);
  myservo.write(0);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  Serial.println("=====================================");
  Serial.println("  SYSTEM READY: SMART LOCK");
  Serial.println("=====================================");
}

void loop() {
  char customKey = customKeypad.getKey();

  if (customKey) {
    Serial.print("Pressed Key: ");
    Serial.println(customKey);

    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    if (customKey == '#') {
      passwordInput = "";
      Serial.println("Input cleared.");

    } else if (customKey == '*') {
      Serial.print("Submitting Password: ");
      Serial.println(passwordInput);

      if (passwordInput == CORRECT_PASSWORD) {
        Serial.println("ACCESS GRANTED!");
        failCount = 0;
        myservo.write(90);
        digitalWrite(PIN_BUZZER, HIGH);
        delay(500);
        digitalWrite(PIN_BUZZER, LOW);
        sendToMobius("UNLOCKED");
        delay(3000);
        myservo.write(0);
        Serial.println("System relocked.");
        sendToMobius("LOCKED");

      } else {
        failCount++;
        Serial.println("ACCESS DENIED! Attempt: " + String(failCount));
        for (int i = 0; i < 3; i++) {
          digitalWrite(PIN_BUZZER, HIGH);
          delay(150);
          digitalWrite(PIN_BUZZER, LOW);
          delay(100);
        }
        sendToMobius("DENIED");

        if (failCount >= 3) {
          lockout();
        }
      }
      passwordInput = "";

    } else {
      passwordInput += customKey;
    }
  }
}