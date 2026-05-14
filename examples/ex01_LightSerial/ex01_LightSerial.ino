/*
 * 練習 01 - 光敏電阻序列埠讀值
 *
 * 光敏電阻 AO 接 GPIO 34，數值直接顯示在 Serial Monitor。
 */

const int PIN_LIGHT = 34;

void setup() {
  Serial.begin(115200);
}

void loop() {
  int value = analogRead(PIN_LIGHT);
  Serial.println(value);
  delay(500);
}
