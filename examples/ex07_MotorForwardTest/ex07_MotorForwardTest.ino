/*
 * 練習 07 - 馬達接線前進測試
 *
 * 功能：
 * - 開機後先煞車 3 秒，方便把車子放到安全位置
 * - 之後維持車體向前移動
 * - 用來檢查四顆馬達的接線位置與轉向是否正確
 *
 * 測試方式：
 * 1. 請先架高車體，避免燒錄後直接衝出去
 * 2. 打開 Serial Monitor，鮑率設定為 115200
 * 3. 確認倒數結束後，四顆輪子應該讓車體向前移動
 * 4. 若車體後退、旋轉或歪斜，請檢查馬達插座位置或 A/B 線方向
 *
 * 安全提醒：
 * - 這個程式會讓車子持續前進，不會自動停下來。
 * - 測試時請先拔高車輪或架高車體，確認方向正確後再放到地面。
 * - 若要停止，請拔掉電源或重新燒錄停止馬達的程式。
 */

// 載入馬達、GoSUMO 車體控制、WS2812B LED 等類別。
#include <smhs_IoT_Robot.h>

// 測試速度 (0 ~ 1023)
// 建議先使用低速測試，確認方向正確後再慢慢調高。
const int TEST_SPEED = 520;

// 建立四顆馬達物件。
// 每顆馬達需要兩個控制腳位：A 與 B。
// 腳位名稱由 SMHS_HW 統一管理，避免在範例中直接寫死 GPIO 號碼。
Motor m1(SMHS_HW::M1A, SMHS_HW::M1B);
Motor m2(SMHS_HW::M2A, SMHS_HW::M2B);
Motor m3(SMHS_HW::M3A, SMHS_HW::M3B);
Motor m4(SMHS_HW::M4A, SMHS_HW::M4B);

// 將四顆馬達交給 GoSUMO 物件管理。
// 之後只要呼叫 robot.act(方向, 速度)，函式庫會自動計算四顆輪子的轉向。
GoSUMO robot(&m1, &m2, &m3, &m4);

// 建立板上 WS2812B 狀態燈。
stateLED robotLED(SMHS_HW::WSLED, 1);

void setup() {
  // 啟動序列埠，輸出倒數與測試狀態。
  Serial.begin(115200);

  // 開機第一件事先停止馬達，避免上一個程式或雜訊造成馬達誤動作。
  robot.stop();

  // 初始化狀態燈。
  robotLED.begin();

  // 設定亮度，60 較不刺眼。
  robotLED.setBrightness(60);

  // 紅燈代表目前仍在倒數等待，車體尚未開始移動。
  robotLED.fillColor(stateLED::RED);

  Serial.println();
  Serial.println("練習 07：馬達接線前進測試");
  Serial.println("請先架高車體，倒數結束後會持續前進。");

  // 倒數 3 秒，讓使用者有時間把手離開車輪，或把車子放到安全位置。
  for (int i = 3; i > 0; i--) {
    Serial.print("倒數 ");
    Serial.print(i);
    Serial.println(" 秒...");
    delay(1000);
  }

  // 綠燈代表測試開始。
  robotLED.fillColor(stateLED::GREEN);

  // 發送前進指令。
  robot.act(GoSUMO::FORWARD, TEST_SPEED);

  Serial.print("開始前進，速度：");
  Serial.println(TEST_SPEED);
}

void loop() {
  // 持續送出前進指令，避免外力或雜訊造成短暫停止後不再動作。
  robot.act(GoSUMO::FORWARD, TEST_SPEED);

  // 每 0.2 秒重送一次即可，不需要以極高頻率重複送指令。
  delay(200);
}
