/*
 * 練習 05 - 光照感測器讀取 (類比輸入)
 *
 * 學習目標：
 * - 認識類比感測器與數位感測器的差異
 * - 學習使用 analogRead() 讀取 0~4095 的 ADC 數值
 * - 學習使用 map() 函式將數值換算成百分比 (0~100%)
 * - 學習使用 constrain() 函式限制數值在合理範圍內
 * - 建立 WiFi 網頁即時顯示感測器資料
 *
 * 接線說明：
 *   光敏電阻模組訊號腳 (AO) → GPIO 34  ← 只能輸入，不能輸出！
 *   光敏電阻模組電源腳 (VCC) → 3.3V
 *   光敏電阻模組接地腳 (GND) → GND
 *
 * ADC 說明：
 *   ESP32 的 ADC 解析度為 12 位元，數值範圍為 0 ~ 4095
 *   光線越強 → 電阻越小 → 電壓越高 → ADC 數值越大
 *   因此：數值越高 = 越亮；數值越低 = 越暗
 *
 * 注意：GPIO 34、35、36、39 為「只能輸入」腳位，不可設為輸出！
 */

// ==================== 1. 標頭檔區 ====================
#include <WiFi.h>
#include <WebServer.h>
#include <smhs_IoT_Robot.h>


// ==================== 2. 宣告區 ====================
// --- 網路設定 (請修改成你的 WiFi 名稱與密碼) ---
const char* ssid     = "your_ssid";
const char* password = "your_password";

// --- 硬體腳位設定 ---
const int PIN_LIGHT = 34;  // 光敏電阻訊號腳接 GPIO 34 (類比輸入)

// --- 感測器數值變數 ---
int lightRaw = 0;    // 原始 ADC 數值 (0 ~ 4095)
int lightPct = 0;    // 換算後百分比 (0 ~ 100 %)

// --- 計時變數 ---
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 500;  // 每 0.5 秒讀取一次

// --- 建立硬體物件 ---
WebServer server(80);
stateLED  robotLED(SMHS_HW::WSLED, 1);

// --- 副程式預先宣告 ---
void connectWiFi();
void readLightSensor();
void handleDashboard();
void handleStatus();


// ==================== 3. 初始化區 ====================
void setup() {
  Serial.begin(115200);
  // GPIO 34 是純輸入腳，不需要 pinMode 設定

  robotLED.begin();
  robotLED.fillColor(stateLED::RED);

  Serial.println("\n練習 05：光照感測器啟動中...");

  connectWiFi();

  server.on("/",       handleDashboard);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("Web Server 已啟動！");
  Serial.println("請在瀏覽器輸入上方 IP 位址查看儀表板\n");
}


// ==================== 4. 主程式區 ====================
void loop() {
  unsigned long currentTime = millis();

  // 每 500ms 讀取一次光照感測器
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    readLightSensor();
    lastReadTime = currentTime;
  }

  // 持續監聽網頁請求
  server.handleClient();
}


// ==================== 5. 副程式區 ====================

// --- 5.1 光照感測器讀取 ---
void readLightSensor() {
  // analogRead() 回傳 0 ~ 4095 的整數
  lightRaw = analogRead(PIN_LIGHT);

  // map(值, 輸入最小, 輸入最大, 輸出最小, 輸出最大)
  // 將 0~4095 的 ADC 原始值線性換算為 0~100 的百分比
  lightPct = map(lightRaw, 0, 4095, 0, 100);

  // constrain() 確保數值不超出 0~100 的合理範圍
  lightPct = constrain(lightPct, 0, 100);

  // 在 Serial 監控視窗顯示
  Serial.print("原始 ADC：");
  Serial.print(lightRaw);
  Serial.print("  →  光照強度：");
  Serial.print(lightPct);
  Serial.println(" %");
}

// --- 5.2 WiFi 連線 ---
void connectWiFi() {
  Serial.print("正在連線到 WiFi：");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    robotLED.fillColor(stateLED::GREEN);
    Serial.println("\nWiFi 連線成功！");
    Serial.print("IP 位址：");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi 連線失敗，請確認 SSID/密碼，並確認是 2.4GHz 頻段。");
  }
}

// --- 5.3 網頁儀表板 ---
void handleDashboard() {
  // 根據光照強度決定燈泡 Emoji
  String icon;
  if      (lightPct >= 70) icon = "☀️";
  else if (lightPct >= 30) icon = "🌤️";
  else                     icon = "🌑";

  String html = "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>練習05 - 光照感測</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;margin:20px;text-align:center;background:#2c3e50;color:#ecf0f1;}";
  html += ".card{background:#34495e;border-radius:10px;padding:25px;margin:15px auto;max-width:300px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  html += ".value{font-size:52px;font-weight:bold;color:#f39c12;margin:10px 0;}";
  html += ".unit{font-size:22px;color:#bdc3c7;}";
  html += ".raw{font-size:14px;color:#7f8c8d;margin-top:8px;}";
  html += ".bar-bg{background:#2c3e50;border-radius:10px;height:20px;margin-top:12px;overflow:hidden;}";
  html += ".bar{background:#f39c12;height:100%;border-radius:10px;transition:width 0.5s;}";
  html += ".btn{background:#2ecc71;color:#fff;padding:12px 24px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin-top:10px;font-weight:bold;}";
  html += ".btn:active{transform:scale(0.95);}";
  html += ".subtitle{color:#95a5a6;font-size:14px;margin-bottom:20px;}";
  html += "</style></head><body>";

  html += "<h1>光照感測器</h1>";
  html += "<p class='subtitle'>練習 05 - smhs_IoT_Robot</p>";

  html += "<div class='card'>";
  html += "<div style='font-size:36px;'>" + icon + "</div>";
  html += "<div class='value' id='val-pct'>" + String(lightPct) + "</div>";
  html += "<div class='unit'>% 光照強度</div>";
  // 進度條
  html += "<div class='bar-bg'><div class='bar' id='val-bar' style='width:" + String(lightPct) + "%;'></div></div>";
  html += "<div class='raw' id='val-raw'>原始 ADC 數值：" + String(lightRaw) + " / 4095</div>";
  html += "</div>";

  html += "<button class='btn' onclick='fetchData()'>刷新數值</button>";

  html += "<script>";
  html += "function fetchData(){";
  html += "  fetch('/status').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('val-pct').innerText = d.pct;";
  html += "    document.getElementById('val-bar').style.width = d.pct + '%';";
  html += "    document.getElementById('val-raw').innerText = '原始 ADC 數值：' + d.raw + ' / 4095';";
  html += "  }).catch(e=>console.log(e));";
  html += "}";
  html += "setInterval(fetchData, 1000);";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- 5.4 感測器數值 JSON API ---
void handleStatus() {
  String json = "{";
  json += "\"raw\":" + String(lightRaw) + ",";
  json += "\"pct\":" + String(lightPct);
  json += "}";
  server.send(200, "application/json", json);
}
