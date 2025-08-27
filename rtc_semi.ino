#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Servo.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo pillServo;

// Alarm times (hour, minute) - adjust as needed
const int alarmTimes[][2] = {
  {21, 59},
  {22, 0},
  {22, 1}
};

// Pins
const int buzzerPin = 7;
const int ledPin = 8;
const int buttonPin = 2;
const int servoPin = 9;

// Servo angles for each alarm
int servoAngles[3] = {60, 120, 180};

// Flags
bool alarmActive[3] = {false, false, false};
bool alarmDismissed[3] = {false, false, false};
bool servoOpened[3] = {false, false, false};  // Track if servo already rotated

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Internal pull-up

  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  pillServo.attach(servoPin);
  pillServo.write(0);  // Initial position

  Serial.begin(9600);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to computer compile time
  }

  // ✅ Temporarily set current time ONCE then comment this out
   rtc.adjust(DateTime(2025, 4, 12, 21, 58, 0));  // <-- Set to your actual current time
}

void loop() {
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  // Check for active alarm
  for (int i = 0; i < 3; i++) {
    if (hour == alarmTimes[i][0] && minute == alarmTimes[i][1] && !alarmDismissed[i]) {
      alarmActive[i] = true;
    }
  }

  // Trigger alarm behavior
  for (int i = 0; i < 3; i++) {
    if (alarmActive[i] && !alarmDismissed[i]) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("** Reminder **");
      lcd.setCursor(0, 1);
      lcd.print("Take medicine!");

      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 1000);

      // Rotate servo to proper compartment if not already done
      if (!servoOpened[i]) {
        pillServo.write(servoAngles[i]);  // Rotate to 0°, 60°, or 180°
        delay(1000);  // Wait for pill to drop
        servoOpened[i] = true;
      }

      // Wait for button press to dismiss
      if (digitalRead(buttonPin) == LOW) {
        alarmDismissed[i] = true;
        alarmActive[i] = false;
        noTone(buzzerPin);
        digitalWrite(ledPin, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Alarm Stopped");
        delay(2000);
      }

      break;  // Handle one alarm at a time
    }
  }

  // Normal display mode
  if (!alarmActive[0] && !alarmActive[1] && !alarmActive[2]) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(now.month()); lcd.print("/");
    lcd.print(now.day()); lcd.print("/");
    lcd.print(now.year());

    lcd.setCursor(0, 1);
    lcd.print((hour < 10) ? "0" : ""); lcd.print(hour); lcd.print(":");
    lcd.print((minute < 10) ? "0" : ""); lcd.print(minute); lcd.print(":");
    lcd.print((second < 10) ? "0" : ""); lcd.print(second);

    // Reset alarms and servoOpen flags for the next day
    for (int i = 0; i < 3; i++) {
      if (minute != alarmTimes[i][1]) {
        alarmDismissed[i] = false;
        servoOpened[i] = false;
      }
    }

    noTone(buzzerPin);
    digitalWrite(ledPin, LOW);
  }

  delay(200);
}
