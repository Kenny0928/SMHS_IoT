/*
 * 練習 00 - WS2812B 三色變換
 *
 * 紅、綠、藍依序亮起，每次亮 500ms，兩色之間穿插熄滅 500ms。
 *
 * 學習目標：
 * - 認識 WS2812B 全彩 LED 的基本控制方式
 * - 學習如何初始化 smhs_IoT_Robot 函式庫提供的 stateLED 物件
 * - 學習 fillColor()、clear() 與 delay() 的搭配
 *
 * 硬體說明：
 * - WS2812B 是一種可控制 RGB 三色亮度的全彩 LED
 * - 本範例只使用板子上的第 1 顆 WS2812B LED
 * - 顏色常數 stateLED::RED / GREEN / BLUE 已由函式庫定義好
 */

// 載入 smhs_IoT_Robot 函式庫，裡面包含 stateLED、Motor、GoSUMO 等硬體控制類別。
#include <smhs_IoT_Robot.h>

// 建立一個 stateLED 物件，名稱叫 led。
// SMHS_HW::WSLED：代表使用開發板上預設的 WS2812B 腳位。
// 1：代表目前只控制 1 顆 LED。
stateLED led(SMHS_HW::WSLED, 1);

void setup() {
  // 初始化 WS2812B LED。使用 LED 前一定要先呼叫 begin()。
  led.begin();

  // 設定 LED 亮度，範圍通常為 0~255。
  // 60 屬於較柔和的亮度，適合課堂測試，避免太刺眼。
  led.setBrightness(60);
}

void loop() {
  // 將整顆 LED 填滿紅色。
  led.fillColor(stateLED::RED);

  // 維持紅燈 500 毫秒，也就是 0.5 秒。
  delay(500);

  // 關閉 LED，讓下一個顏色切換時更明顯。
  led.clear();
  delay(500);

  // 將 LED 改成綠色。
  led.fillColor(stateLED::GREEN);
  delay(500);

  // 再次熄滅 0.5 秒。
  led.clear();
  delay(500);

  // 將 LED 改成藍色。
  led.fillColor(stateLED::BLUE);
  delay(500);

  // 熄滅後 loop() 會重新回到最上方，繼續紅、綠、藍循環。
  led.clear();
  delay(500);
}
