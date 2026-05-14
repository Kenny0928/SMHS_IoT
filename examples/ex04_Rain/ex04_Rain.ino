/*
 * 練習 04 - 雨水感測器讀取 (數位輸入)
 *
 * 學習目標：
 * - 認識數位感測器的工作原理 (HIGH / LOW 訊號)
 * - 學習使用 digitalRead() 讀取感測器狀態
 * - 了解感測器模組上的靈敏度旋鈕如何調整觸發臨界值
 * - 建立 WiFi 網頁即時顯示感測器狀態
 *
 * 接線說明：
 *   雨水模組數位腳 (DO) → GPIO 27
 *   雨水模組電源腳 (VCC) → 3.3V
 *   雨水模組接地腳 (GND) → GND
 *
 * 感測器邏輯：
 *   無雨 (乾燥) → 模組輸出 HIGH (1)
 *   有雨 (潮濕) → 模組輸出 LOW  (0)
 *   ※ 此為「Active Low」設計，注意判斷邏輯是反向的！
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
const int PIN_RAIN = 27;  // 雨水感測器數位腳接 GPIO 27

// --- 感測器數值變數 ---
// true = 有雨, false = 無雨
bool isRaining = false;
int  rawValue  = 0;  // 原始數位讀值 (0 或 1)

// --- 計時變數 ---
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 500;  // 每 0.5 秒讀取一次

// --- 建立硬體物件 ---
WebServer server(80);
stateLED  robotLED(SMHS_HW::WSLED, 1);

// --- 副程式預先宣告 ---
void connectWiFi();
void readRainSensor();
void handleDashboard();
void handleStatus();


// ==================== 3. 初始化區 ====================
void setup() {
  Serial.begin(115200);

  // 設定雨水感測器腳位為輸入模式
  pinMode(PIN_RAIN, INPUT);

  robotLED.begin();
  robotLED.fillColor(stateLED::RED);  // 開機亮紅燈

  Serial.println("\n練習 04：雨水感測器啟動中...");

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

  // 每 500ms 讀取一次雨水感測器
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    readRainSensor();
    lastReadTime = currentTime;
  }

  // 持續監聽網頁請求
  server.handleClient();
}


// ==================== 5. 副程式區 ====================

// --- 5.1 雨水感測器讀取 ---
void readRainSensor() {
  // digitalRead() 回傳 HIGH(1) 或 LOW(0)
  rawValue = digitalRead(PIN_RAIN);

  // 雨水模組為 Active Low：有雨時輸出 LOW，無雨時輸出 HIGH
  // 因此用 != HIGH 或 == LOW 來判斷「有雨」
  isRaining = (rawValue == LOW);

  // 在 Serial 監控視窗顯示
  Serial.print("原始數值: ");
  Serial.print(rawValue);
  Serial.print("  →  狀態: ");
  Serial.println(isRaining ? "有雨 ☔" : "無雨 ☀");
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
  // 根據當前狀態決定顯示顏色與文字
  String statusColor = isRaining ? "#e74c3c" : "#2ecc71";
  String statusText  = isRaining ? "有雨 ☔" : "無雨 ☀";
  String statusDesc  = isRaining ? "感測到水分，請注意！" : "目前環境乾燥，正常。";

  String html = "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>練習04 - 雨水感測</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;margin:20px;text-align:center;background:#2c3e50;color:#ecf0f1;}";
  html += ".card{background:#34495e;border-radius:10px;padding:25px;margin:15px auto;max-width:300px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  html += ".status{font-size:52px;font-weight:bold;margin:10px 0;}";
  html += ".desc{font-size:16px;color:#bdc3c7;margin-top:8px;}";
  html += ".raw{font-size:14px;color:#7f8c8d;margin-top:5px;}";
  html += ".btn{background:#2ecc71;color:#fff;padding:12px 24px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin-top:10px;font-weight:bold;}";
  html += ".btn:active{transform:scale(0.95);}";
  html += ".subtitle{color:#95a5a6;font-size:14px;margin-bottom:20px;}";
  html += "</style></head><body>";

  html += "<h1>雨水感測器</h1>";
  html += "<p class='subtitle'>練習 04 - smhs_IoT_Robot</p>";

  html += "<div class='card'>";
  html += "<div class='status' id='val-status' style='color:" + statusColor + ";'>" + statusText + "</div>";
  html += "<div class='desc'  id='val-desc'>"  + statusDesc  + "</div>";
  html += "<div class='raw'   id='val-raw'>原始數值 (DO)：" + String(rawValue) + "</div>";
  html += "</div>";

  html += "<button class='btn' onclick='fetchData()'>刷新狀態</button>";

  html += "<script>";
  html += "function fetchData(){";
  html += "  fetch('/status').then(r=>r.json()).then(d=>{";
  html += "    let color = d.rain ? '#e74c3c' : '#2ecc71';";
  html += "    let text  = d.rain ? '有雨 ☔' : '無雨 ☀';";
  html += "    let desc  = d.rain ? '感測到水分，請注意！' : '目前環境乾燥，正常。';";
  html += "    document.getElementById('val-status').style.color = color;";
  html += "    document.getElementById('val-status').innerText = text;";
  html += "    document.getElementById('val-desc').innerText   = desc;";
  html += "    document.getElementById('val-raw').innerText    = '原始數值 (DO)：' + d.raw;";
  html += "  }).catch(e=>console.log(e));";
  html += "}";
  html += "setInterval(fetchData, 1000);";  // 每秒自動刷新
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- 5.4 感測器數值 JSON API ---
void handleStatus() {
  String json = "{";
  json += "\"rain\":"  + String(isRaining ? "true" : "false") + ",";
  json += "\"raw\":"   + String(rawValue);
  json += "}";
  server.send(200, "application/json", json);
}
