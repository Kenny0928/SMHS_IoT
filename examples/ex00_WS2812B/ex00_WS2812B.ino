/*
 * 練習 00 - WS2812B 三色變換
 *
 * 紅、綠、藍依序亮起，每次亮 500ms，兩色之間穿插熄滅 500ms。
 */

#include <smhs_IoT_Robot.h>

stateLED led(SMHS_HW::WSLED, 1);

void setup() {
  led.begin();
  led.setBrightness(60);
}

void loop() {
  led.fillColor(stateLED::RED);
  delay(500);
  led.clear();
  delay(500);

  led.fillColor(stateLED::GREEN);
  delay(500);
  led.clear();
  delay(500);

  led.fillColor(stateLED::BLUE);
  delay(500);
  led.clear();
  delay(500);
}
