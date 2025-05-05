#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>

//------------------------------
#define CHECK_BTN_PIN D4
#define BUZZ_PIN D5
#define COMBO_PIN D3
#define LID_OPEN_PIN D6
#define LOCK_PIN D8
#define LED_DATA_PIN D2
#define NUM_LEDS 4
#define LOADING_INTERVAL 1000
#define BUZZER_INTERVAL 2000
#define LOCK_OPEN_INTERVAL 1000
#define MAX_LATCH_RETRIES 5
//------------------------------
CRGB leds[NUM_LEDS];
bool isLidOpen;
bool openLatch;
int latchOpenTries;
unsigned long lastInterruptTime;
//------------------------------

//------------------------------
void softDelay(unsigned long millis) {
  ESP.wdtFeed();
  delay(millis);
}
//------------------------------
void reset() {
  latchOpenTries = 0;
  FastLED.clear(true);
}
//------------------------------
void showComboState(bool comboOk) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = comboOk ? CRGB::Red : CRGB::Green;
  }
  FastLED.show();
}
//------------------------------
void showLoadingSeq() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
    FastLED.show();
    softDelay(LOADING_INTERVAL);
  }
}
//------------------------------
void openLidAlarm() {
  Serial.println("Cheater alert!");
  showComboState(false);
  digitalWrite(BUZZ_PIN, HIGH);
  softDelay(600);
  FastLED.clear(true);
  digitalWrite(BUZZ_PIN, LOW);
  softDelay(600);
}
//------------------------------
void IRAM_ATTR checkLidStatus() {
  if ((millis() - lastInterruptTime) >= 200) {
    isLidOpen = (digitalRead(LID_OPEN_PIN) == HIGH);
    if (isLidOpen) {
      Serial.println("Lid was opened");
    } else {
      Serial.println("Lid was closed");
      openLatch = false;
    }
  }
  lastInterruptTime = millis();
}
//------------------------------
void IRAM_ATTR checkCombination() {
  Serial.println("Button pressed");
  if (isLidOpen) {
    Serial.println("Lid is open");
    return;
  }
  bool comboOk = (digitalRead(COMBO_PIN) == LOW);
  FastLED.clear(true);
  softDelay(LOADING_INTERVAL/2);
  showLoadingSeq();
  showComboState(comboOk);
  if (comboOk) {
    Serial.println("Combination ok");
    openLatch = true;
  } else {
    Serial.println("Combination bad");
    digitalWrite(BUZZ_PIN, HIGH);
    softDelay(BUZZER_INTERVAL);
    digitalWrite(BUZZ_PIN, LOW);
    FastLED.clear(true);
  }
}
//------------------------------
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  
  pinMode(CHECK_BTN_PIN, INPUT_PULLUP);
  pinMode(COMBO_PIN, INPUT_PULLUP);
  pinMode(LID_OPEN_PIN, INPUT_PULLUP);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);

  FastLED.addLeds<WS2812B, LED_DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear(true);

  isLidOpen = true;
  openLatch = true;

  attachInterrupt(CHECK_BTN_PIN, checkCombination, FALLING);
  attachInterrupt(LID_OPEN_PIN, checkLidStatus, CHANGE);

  Serial.println("Ready");
}
//------------------------------
void loop() {
  if (isLidOpen && !openLatch) {
    // someone is cheating!
    openLidAlarm();
  }

  if (openLatch && !isLidOpen) {
    if (latchOpenTries < MAX_LATCH_RETRIES) {
      digitalWrite(LOCK_PIN, HIGH);
      softDelay(LOCK_OPEN_INTERVAL);
      digitalWrite(LOCK_PIN, LOW);
      softDelay(LOCK_OPEN_INTERVAL/2);
      latchOpenTries += 1;
      if (digitalRead(LID_OPEN_PIN) == HIGH) reset();
    } else {
      reset();
      openLatch = false;
    }
  }
}
//------------------------------