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
 */

#include <smhs_IoT_Robot.h>

// 測試速度 (0 ~ 1023)
// 建議先使用低速測試，確認方向正確後再慢慢調高。
const int TEST_SPEED = 520;

Motor m1(SMHS_HW::M1A, SMHS_HW::M1B);
Motor m2(SMHS_HW::M2A, SMHS_HW::M2B);
Motor m3(SMHS_HW::M3A, SMHS_HW::M3B);
Motor m4(SMHS_HW::M4A, SMHS_HW::M4B);
GoSUMO robot(&m1, &m2, &m3, &m4);

stateLED robotLED(SMHS_HW::WSLED, 1);

void setup() {
  Serial.begin(115200);

  robot.stop();

  robotLED.begin();
  robotLED.setBrightness(60);
  robotLED.fillColor(stateLED::RED);

  Serial.println();
  Serial.println("練習 07：馬達接線前進測試");
  Serial.println("請先架高車體，倒數結束後會持續前進。");

  for (int i = 3; i > 0; i--) {
    Serial.print("倒數 ");
    Serial.print(i);
    Serial.println(" 秒...");
    delay(1000);
  }

  robotLED.fillColor(stateLED::GREEN);
  robot.act(GoSUMO::FORWARD, TEST_SPEED);

  Serial.print("開始前進，速度：");
  Serial.println(TEST_SPEED);
}

void loop() {
  // 持續送出前進指令，避免外力或雜訊造成短暫停止後不再動作。
  robot.act(GoSUMO::FORWARD, TEST_SPEED);
  delay(200);
}
