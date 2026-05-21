/*
 * 課程 03 - 條件與迴圈：光敏電阻控制藍燈
 *
 * 功能：
 * - 持續讀取光敏電阻的類比數值
 * - 當光線數值高於門檻值 THRESHOLD 時，點亮藍色 WS2812B LED
 * - 當光線數值低於或等於門檻值時，關閉 LED
 *
 * 學習目標：
 * - 練習 if / else 條件判斷
 * - 練習 loop() 反覆執行的特性
 * - 學習把感測器輸入轉換成 LED 輸出
 *
 * 調整方式：
 * - 如果 LED 太容易亮，請把 THRESHOLD 調高。
 * - 如果 LED 太不容易亮，請把 THRESHOLD 調低。
 */

// 載入 smhs_IoT_Robot 函式庫，用來控制板上的 WS2812B LED。
#include <smhs_IoT_Robot.h>

// 光敏電阻模組 AO 接到 GPIO 34。
const int PIN_LIGHT = 34;

// 判斷亮暗的門檻值。
// ESP32 類比讀值約為 0~4095，500 是範例起始值，實際可依環境光調整。
const int THRESHOLD = 500;

// 建立 LED 控制物件，只控制板上的 1 顆 WS2812B。
stateLED led(SMHS_HW::WSLED, 1);

void setup() {
  // 啟動序列埠，方便觀察光敏電阻目前讀值。
  Serial.begin(115200);

  // 初始化 LED，使用 fillColor() 或 clear() 前必須先呼叫。
  led.begin();
}

void loop() {
  // 讀取目前光線強度。
  int value = analogRead(PIN_LIGHT);

  // 印出原始數值，方便判斷 THRESHOLD 應該設定多少。
  Serial.println(value);

  // 如果光線讀值大於門檻值，就點亮藍燈。
  if (value > THRESHOLD) {
    led.fillColor(stateLED::BLUE);
  } else {
    // 否則關閉 LED。
    led.clear();
  }

  // 每 0.1 秒檢查一次。時間越短反應越快，但 Serial Monitor 資料也會越多。
  delay(100);
}
