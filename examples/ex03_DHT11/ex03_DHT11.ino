/*
 * 練習 03 - 溫濕度感測器讀取 (DHT11)
 *
 * 學習目標：
 * - 認識 DHT11 溫濕度感測器的接線方式
 * - 學習使用 DHT 函式庫讀取溫度與濕度數值
 * - 透過 Serial 監控視窗觀察即時數值
 * - 建立 WiFi 網頁即時顯示感測器資料
 *
 * 接線說明：
 *   DHT11 資料腳 (DATA) → GPIO 4
 *   DHT11 電源腳 (VCC)  → 3.3V
 *   DHT11 接地腳 (GND)  → GND
 *
 * 注意：DHT11 最快每 2 秒才能讀取一次，讀取太頻繁會得到錯誤值！
 */

// ==================== 1. 標頭檔區 ====================
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <smhs_IoT_Robot.h>


// ==================== 2. 宣告區 ====================
// --- 網路設定 (請修改成你的 WiFi 名稱與密碼) ---
const char* ssid     = "your_ssid";
const char* password = "your_password";

// --- 硬體腳位設定 ---
const int PIN_DHT = 4;  // DHT11 資料腳接 GPIO 4

// --- 感測器數值變數 ---
float temperature = 0.0;  // 溫度 (°C)
float humidity    = 0.0;  // 濕度 (%)

// --- 計時變數 (非阻斷式定時讀取，不使用 delay) ---
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000;  // DHT11 最快讀取速率：2000 毫秒

// --- 建立硬體物件 ---
DHT       dht(PIN_DHT, DHT11);
WebServer server(80);
stateLED  robotLED(SMHS_HW::WSLED, 1);

// --- 副程式預先宣告 ---
void connectWiFi();
void readDHT11();
void handleDashboard();
void handleStatus();


// ==================== 3. 初始化區 ====================
void setup() {
  Serial.begin(115200);
  dht.begin();

  robotLED.begin();
  robotLED.fillColor(stateLED::RED);  // 開機亮紅燈，等待 WiFi 連線

  Serial.println("\n練習 03：DHT11 溫濕度感測器啟動中...");

  connectWiFi();

  // 設定網頁路由
  server.on("/",       handleDashboard);  // 主頁：顯示儀表板
  server.on("/status", handleStatus);     // API：回傳 JSON 資料
  server.begin();

  Serial.println("Web Server 已啟動！");
  Serial.println("請在瀏覽器輸入上方 IP 位址查看儀表板\n");
}


// ==================== 4. 主程式區 ====================
void loop() {
  unsigned long currentTime = millis();

  // 每 2 秒讀取一次 DHT11 (非阻斷式，不會卡住程式)
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    readDHT11();
    lastReadTime = currentTime;
  }

  // 持續監聽網頁請求
  server.handleClient();
}


// ==================== 5. 副程式區 ====================

// --- 5.1 DHT11 感測器讀取 ---
void readDHT11() {
  float h = dht.readHumidity();     // 讀取濕度
  float t = dht.readTemperature();  // 讀取溫度 (預設攝氏)

  // isnan() 用來檢查是否為「無效數值」(Not a Number)
  // 若感測器沒接好，或讀取太頻繁，會回傳 NaN
  if (isnan(h) || isnan(t)) {
    Serial.println("⚠️  DHT11 讀取失敗！請確認接線是否正確。");
    return;  // 讀取失敗就跳出，不更新數值
  }

  // 更新全域變數
  temperature = t;
  humidity    = h;

  // 在 Serial 監控視窗顯示
  Serial.print("溫度: ");
  Serial.print(temperature, 1);
  Serial.print(" °C  |  濕度: ");
  Serial.print(humidity, 1);
  Serial.println(" %");
}

// --- 5.2 WiFi 連線 ---
void connectWiFi() {
  Serial.print("正在連線到 WiFi：");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  // 最多等候 20 秒鐘
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    robotLED.fillColor(stateLED::GREEN);  // 連線成功，亮綠燈
    Serial.println("\nWiFi 連線成功！");
    Serial.print("IP 位址：");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi 連線失敗，請確認 SSID/密碼，並確認是 2.4GHz 頻段。");
  }
}

// --- 5.3 網頁儀表板 ---
void handleDashboard() {
  String html = "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>練習03 - DHT11 溫濕度</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;margin:20px;text-align:center;background:#2c3e50;color:#ecf0f1;}";
  html += ".card{background:#34495e;border-radius:10px;padding:20px;margin:15px auto;max-width:300px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  html += ".value{font-size:52px;font-weight:bold;color:#3498db;margin:10px 0;}";
  html += ".unit{font-size:22px;color:#bdc3c7;}";
  html += ".label{font-size:18px;color:#95a5a6;margin-bottom:5px;}";
  html += ".btn{background:#2ecc71;color:#fff;padding:12px 24px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin-top:10px;font-weight:bold;}";
  html += ".btn:active{transform:scale(0.95);}";
  html += "h1{margin-bottom:5px;}";
  html += ".subtitle{color:#95a5a6;font-size:14px;margin-bottom:20px;}";
  html += "</style></head><body>";

  html += "<h1>DHT11 溫濕度感測</h1>";
  html += "<p class='subtitle'>練習 03 - smhs_IoT_Robot</p>";

  // 溫度卡片
  html += "<div class='card'>";
  html += "<div class='label'>溫度 Temperature</div>";
  html += "<div class='value' id='val-temp'>" + String(temperature, 1) + "</div>";
  html += "<div class='unit'>°C</div>";
  html += "</div>";

  // 濕度卡片
  html += "<div class='card'>";
  html += "<div class='label'>濕度 Humidity</div>";
  html += "<div class='value' id='val-hum'>" + String(humidity, 1) + "</div>";
  html += "<div class='unit'>%</div>";
  html += "</div>";

  // 手動刷新按鈕 + AJAX 自動更新
  html += "<button class='btn' onclick='fetchData()'>刷新數值</button>";

  html += "<script>";
  html += "function fetchData(){";
  html += "  fetch('/status').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('val-temp').innerText = d.temp.toFixed(1);";
  html += "    document.getElementById('val-hum').innerText = d.hum.toFixed(1);";
  html += "  }).catch(e=>console.log(e));";
  html += "}";
  // 每 3 秒自動刷新一次
  html += "setInterval(fetchData, 3000);";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- 5.4 感測器數值 JSON API ---
// 網頁的 JavaScript 會呼叫此路由取得最新數值
void handleStatus() {
  String json = "{";
  json += "\"temp\":" + String(temperature, 1) + ",";
  json += "\"hum\":"  + String(humidity, 1);
  json += "}";
  server.send(200, "application/json", json);
}
