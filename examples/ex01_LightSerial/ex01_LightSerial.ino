/*
 * 練習 01 - 光敏電阻序列埠讀值
 *
 * 光敏電阻 AO 接 GPIO 34，數值直接顯示在 Serial Monitor。
 *
 * 學習目標：
 * - 認識類比輸入 analogRead()
 * - 學習使用 Serial Monitor 觀察感測器資料
 * - 了解 ESP32 ADC 讀值範圍約為 0~4095
 *
 * 接線說明：
 *   光敏電阻模組 AO  → GPIO 34
 *   光敏電阻模組 VCC → 3.3V
 *   光敏電阻模組 GND → GND
 *
 * 注意：
 * - GPIO 34 是 ESP32 的「只能輸入」腳位，適合讀取感測器。
 * - 不同光敏電阻模組的電路可能不同，亮暗與數值高低可能需要實測確認。
 */

// 光敏電阻的類比輸出 AO 接到 GPIO 34。
// const 代表這個腳位設定在程式執行期間不會被修改。
const int PIN_LIGHT = 34;

void setup() {
  // 啟動序列埠，鮑率設定為 115200。
  // Arduino IDE 的 Serial Monitor 也要選 115200，才會正常顯示文字。
  Serial.begin(115200);
}

void loop() {
  // analogRead() 讀取類比電壓，ESP32 預設回傳 0~4095。
  // 數值越大通常代表輸入電壓越高。
  int value = analogRead(PIN_LIGHT);

  // 將讀到的數值印到 Serial Monitor，每次印完自動換行。
  Serial.println(value);

  // 每 0.5 秒讀取一次，避免資料刷太快難以觀察。
  delay(500);
}
