#include <Arduino.h>
#include <TFT_eSPI.h>
#include "TraditionalChinese16.h"
TFT_eSPI tft;
void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH); // Direct backlight drive: no PWM, no automatic sleep.
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLUE);
  tft.fillRect(0, 0, 106, 35, TFT_RED);
  tft.fillRect(106, 0, 107, 35, TFT_GREEN);
  tft.fillRect(213, 0, 107, 35, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("DISPLAY TEST", 160, 65, 4);
  tft.loadFont(TraditionalChinese16);
  tft.drawString("繁體中文介面", 160, 110);
  tft.drawString("頻寬  儲存電台  選單", 160, 140);
  tft.unloadFont();
  digitalWrite(38, HIGH);
  Serial.printf("[DISPLAY-TEST] backlight_gpio38=%d ID04=%08lX status09=%08lX\n", digitalRead(38), tft.readcommand32(0x04), tft.readcommand32(0x09));
}
void loop() {
  if (Serial.available()) {
    Serial.read();
    Serial.printf("[DISPLAY-TEST] alive uptime_ms=%lu backlight_gpio38=%d\n", millis(), digitalRead(38));
  }
  delay(10);
}
