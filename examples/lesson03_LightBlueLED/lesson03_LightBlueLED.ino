/*
 * 課程 03 - 條件與迴圈：光敏電阻控制藍燈
 */

#include <smhs_IoT_Robot.h>

const int PIN_LIGHT = 34;
const int THRESHOLD = 500;

stateLED led(SMHS_HW::WSLED, 1);

void setup() {
  Serial.begin(115200);
  led.begin();
}

void loop() {
  int value = analogRead(PIN_LIGHT);
  Serial.println(value);

  if (value > THRESHOLD) {
    led.fillColor(stateLED::BLUE);
  } else {
    led.clear();
  }

  delay(100);
}
