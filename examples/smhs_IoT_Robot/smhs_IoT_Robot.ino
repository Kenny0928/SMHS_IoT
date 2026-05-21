/*
 * ESP32 Multi-Function System - smhs_IoT_Robot 整合版 (進階雙核心版)
 *作者：Adrian
 *日期：2026-05-12
 * 功能：
 * - 核心分離架構 (FreeRTOS Dual-Core)：
 *   - Core 1 (預設 loop)：專心負責高速的感測器讀取與馬達運作。
 *   - Core 0 (NetworkTask)：負責 WiFi、儀表板伺服器、耗時的 ThingSpeak 上傳，互不干擾！
 * - 採用 JS Fetch API 無縫非同步控制，擁有近乎零延遲的手把手感。
 * - 採用 STA 模式 (連接至 Router/手機熱點)
 *
 * 這個範例整合：
 * - DHT11 溫濕度感測器
 * - 雨水感測器
 * - 光敏電阻
 * - GPS 模組
 * - 四輪車體控制
 * - 網頁儀表板
 * - ThingSpeak 雲端上傳
 *
 * 程式架構重點：
 * - loop() 保留在 Core 1，負責需要即時反應的馬達、感測器與 GPS。
 * - networkTaskCode() 固定在 Core 0，負責 WiFi、WebServer、ThingSpeak。
 * - 這樣可以避免雲端上傳或網頁請求造成馬達控制卡頓。
 */

// ==================== 1. 標頭檔區 ====================
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <TinyGPSPlus.h>
#include <WebServer.h>
#include <smhs_IoT_Robot.h>


// ==================== 2. 宣告區 ====================
// --- 網路與 IoT 設定 ---
// ssid / password 請改成現場可用的 2.4GHz WiFi 或手機熱點。
// ESP32 多數開發板不支援 5GHz WiFi，若一直連不上請優先檢查頻段。
const char* ssid = "your WiFi SSID"; // 你的 WiFi SSID
const char* password = "your WiFi password"; // 你的 WiFi 密碼

// 裝置代號會放進 /status JSON，方便多台車同時上線時辨識資料來源。
const char* device_id = "ROBOT-00X";

// --- ThingSpeak 上傳設定 ---
// false：只開啟本機網頁儀表板，不上傳 ThingSpeak。
// true：會依 uploadInterval 定期上傳資料，使用前必須填入 writeAPIKey。
bool enableThingSpeak = false; 

// ThingSpeak 更新資料使用的伺服器網址。
const char* thingSpeakServer = "http://api.thingspeak.com/update";

// ThingSpeak Channel 的 Write API Key。
// 請勿把正式金鑰公開上傳到網路。
const char* writeAPIKey = "YOUR_API_KEY";    

// lastUploadTime 用來記錄上次上傳時間。
unsigned long lastUploadTime = 0;

// ThingSpeak 免費服務通常不適合高頻率上傳，這裡設定每 30 秒一次。
const unsigned long uploadInterval = 30000; // 30秒更新一次

// 感測器讀取也使用 millis() 定時，避免用 delay() 阻塞馬達與 GPS。
unsigned long lastSensorReadTime = 0;
const unsigned long sensorReadInterval = 1000; // 儀表板感測器每秒更新

// --- 硬體腳位與通訊設定 ---
const int PIN_DHT = 4;          // DHT11 溫濕度
const int PIN_RAIN = 27;        // 雨水偵測 (數位)
const int PIN_LIGHT = 34;       // 光照度 (類比)

// GPS 使用 ESP32 的第二組硬體序列埠 HardwareSerial(1)。
// GPS TX 應接到 ESP32 RX 腳；GPS RX 應接到 ESP32 TX 腳。
const int PIN_GPS_RX = 16;
const int PIN_GPS_TX = 17;

// --- 小車遙控速度設定 (統一於此調整) ---
int robotSpeed = 570;           

// --- 防延遲與自動煞車 (網路看門狗) ---
// volatile 表示這些變數可能被不同核心的任務同時讀寫。
// lastCmdTime 由 Core 0 的網頁指令處理更新，Core 1 的 loop() 會拿來判斷是否逾時。
volatile unsigned long lastCmdTime = 0;
const unsigned long CMD_TIMEOUT = 500; // 500毫秒無心跳即視為斷線

// isMoving 代表目前車體是否處於移動狀態。
// 只有移動時才需要看門狗自動煞車，已停止時不用重複 stop()。
volatile bool isMoving = false;

// --- 建立硬體物件 ---
// 四顆馬達各有 A/B 兩個控制腳位。
// 腳位由 SMHS_HW 定義，保持範例程式與硬體版本解耦。
Motor m1(SMHS_HW::M1A, SMHS_HW::M1B);
Motor m2(SMHS_HW::M2A, SMHS_HW::M2B);
Motor m3(SMHS_HW::M3A, SMHS_HW::M3B);
Motor m4(SMHS_HW::M4A, SMHS_HW::M4B);

// GoSUMO 將四顆馬達包裝成車體控制物件，可直接指定前進、後退、左轉、右轉。
GoSUMO robot(&m1, &m2, &m3, &m4);

stateLED robotLED(SMHS_HW::WSLED, 1); // 實例化一顆 ws2812b 全彩 LED

// 若未來需要超音波或循線，可在此宣告：
// HCSR04 ultrasonic(13, 5); 
// IR3CH irSensor(35, 39, 21);

// --- 全域資料結構 ---
// 用 struct 把儀表板需要的感測資料集中管理。
// WebServer 回應 JSON 時會讀取 currentData；感測器更新時會寫入 currentData。
struct SensorData {
  float temperature;
  float humidity;
  int illuminance;
  int rainDetection;
};
SensorData currentData;

// 建立各硬體/服務物件。
DHT dht(PIN_DHT, DHT11);
TinyGPSPlus gps;
HardwareSerial GPS(1);
WebServer server(80);

TaskHandle_t NetworkTask; // 雙核心任務指針

// --- 副程式預先宣告 ---
void connectWiFi();
void readSensors();
void uploadData();
void handleStatus();
void handleDashboard();
void handleCommand();
void networkTaskCode(void * pvParameters);


// ==================== 3. 初始化區 ====================
void setup() {
  // Serial 用於除錯：WiFi 狀態、IP、看門狗訊息、ThingSpeak 上傳結果都會印在這裡。
  Serial.begin(115200);

  // 雨水感測器 DO 腳輸出 HIGH 或 LOW，因此設定為數位輸入。
  pinMode(PIN_RAIN, INPUT);
  
  // 啟動 DHT11。
  dht.begin();

  // 啟動 GPS 序列埠：鮑率 9600，使用 8N1 格式，指定 RX/TX 腳位。
  GPS.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  
  // 開機時先確保馬達停止，避免剛上電時車體誤動作。
  robot.stop();
  
  robotLED.begin();
  robotLED.fillColor(stateLED::RED); // 剛開機點亮成紅燈
  
  Serial.println("\nsmhs_IoT_Robot 雙核心多工系統啟動中...");

  // 將「網路連線與伺服器」的繁重任務指派給另一顆閒置核心 (Core 0)
  // xTaskCreatePinnedToCore() 是 FreeRTOS 提供的任務建立函式。
  // Arduino 的 loop() 通常跑在 Core 1；這裡額外建立 NetworkTask 跑在 Core 0。
  xTaskCreatePinnedToCore(
    networkTaskCode,   // 指向另外寫好的任務函式
    "NetworkTask",     // 任務名稱
    10000,             // 分配記憶體 (Stack Size)
    NULL,              // 傳遞參數
    1,                 // 優先級 (Priority 1)
    &NetworkTask,      // 保存指針
    0                  // 指定在 Core 0 運行
  );
}


// ==================== 4. 主程式區 ====================
// 高速運作區 (Core 1)
void loop() {
  unsigned long currentTime = millis();

  // --- 1. 防延遲看門狗 (Auto-Stop Watchdog) ---
  // 若小車正在移動，且距離上次收到信號已經超過 500ms，強制煞車！
  if (isMoving && (currentTime - lastCmdTime > CMD_TIMEOUT)) {
    robot.stop();
    isMoving = false;
    Serial.println("⚠️ [Core 1] 網路逾時判定：看門狗已強制煞車！");
  }

  // 2. 每秒定期更新感測器資料 (寫入全域變數做運算)
  if (currentTime - lastSensorReadTime >= sensorReadInterval) {
    readSensors();
    lastSensorReadTime = currentTime;
  }

  // 2. 高速捕捉 GPS 資料
  // GPS 模組會不斷從序列埠送出 NMEA 文字資料。
  // TinyGPSPlus 需要持續吃進每個字元，才有機會解析出定位、衛星數量等資訊。
  while (GPS.available()) { gps.encode(GPS.read()); }

  // 3. (保留位置) 未來的超音波或 IR 循線代碼放在這！
  // 由於不會被 WiFi 或 Thingspeak 中斷，此區的硬體感測反應會達到毫秒級零延遲。
  
  // 避免系統空轉過載，加入短暫休息
  delay(10); 
}


// ==================== 5. 副程式區 ====================

// --- 5.1 [Core 0] 新大腦：網路與上傳無限迴圈 ---
void networkTaskCode(void * pvParameters) {
  // Core 0 負責所有網路相關工作，避免 HTTP 請求阻塞 Core 1 的馬達控制。

  // 設定 WiFi 連線
  WiFi.mode(WIFI_STA);
  connectWiFi();

  // 啟動網頁伺服器
  // "/" 是瀏覽器看到的主頁。
  // "/cmd" 接收搖桿方向指令。
  // "/status" 回傳感測器與 GPS JSON 資料。
  server.on("/", handleDashboard);
  server.on("/cmd", handleCommand);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Core 0: Web Server 已經完美啟動在背景運作！");

  bool lastWiFiConnected = false; // 記錄前次 WiFi 狀態

  // Core 0 的專屬無限多工迴圈
  for(;;) {
    // 隨時接聽手機網頁發送出來的 AJAX 控制請求
    server.handleClient();

    // 隨時檢查 WiFi 狀態並更新狀態燈 (連線時綠燈，斷線時紅燈)
    bool currentWiFiConnected = (WiFi.status() == WL_CONNECTED);
    if (currentWiFiConnected != lastWiFiConnected) {
      if (currentWiFiConnected) {
        robotLED.fillColor(stateLED::GREEN);
      } else {
        robotLED.fillColor(stateLED::RED);
      }
      lastWiFiConnected = currentWiFiConnected;
    }

    // 處理耗時約兩秒鐘的 ThingSpeak 網路傳輸
    // ThingSpeak 上傳可能需要等待 DNS、TCP、HTTP 回應。
    // 放在 Core 0 後，即使上傳變慢，也不會直接卡住 Core 1 的車體安全控制。
    if (enableThingSpeak) {
      if (WiFi.status() == WL_CONNECTED && millis() - lastUploadTime >= uploadInterval) {
        uploadData(); 
        lastUploadTime = millis();
      }
    }

    // 讓 FreeRTOS 處理系統底層調度，防看門狗當機
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

// --- 5.2 網頁搖桿非同步控制 ---
void handleCommand() {
  // 瀏覽器會送出 /cmd?dir=F、/cmd?dir=B、/cmd?dir=L、/cmd?dir=R、/cmd?dir=S。
  // dir 是 direction 的縮寫。
  if (server.hasArg("dir")) {
    String dir = server.arg("dir");
    
    // 每次收到任何指令，立刻刷新看門狗時間！
    lastCmdTime = millis();
    
    // 直接改變硬體馬達狀態，Core 1 會瞬間接管動作
    if (dir == "F") { robot.act(GoSUMO::FORWARD, robotSpeed); isMoving = true; }
    else if (dir == "B") { robot.act(GoSUMO::BACKWARD, robotSpeed); isMoving = true; }
    else if (dir == "L") { robot.act(GoSUMO::TURN_LEFT, robotSpeed); isMoving = true; }
    else if (dir == "R") { robot.act(GoSUMO::TURN_RIGHT, robotSpeed); isMoving = true; }
    else if (dir == "S") { robot.stop(); isMoving = false; }
  }
  
  // 背景回報成功，不重整重新載入 ( AJAX 特性)
  server.send(200, "text/plain", "OK");
}

// --- 5.3 網頁儀表板 HTML (使用 AJAX 無縫升級) ---
void handleDashboard() {
  // 這裡直接用 String 組出 HTML/CSS/JavaScript。
  // 好處是不用額外放置網頁檔，燒錄單一 .ino 就能使用。
  String html = "<!doctype html><html><head><meta charset='utf-8'>";
  // 禁止畫面縮放，強化觸控搖桿體驗
  html += "<meta name='viewport' content='width=device-width, user-scalable=no, initial-scale=1'>";
  html += "<title>Robot Pro Dashboard</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;margin:10px;text-align:center;background:#2c3e50;color:#ecf0f1; touch-action:manipulation;}";
  html += ".card{background:#34495e;border-radius:10px;padding:15px;margin:10px auto;max-width:320px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  html += ".sensor{text-align:left; line-height:1.5;}";
  // 按鈕防文字反白
  html += ".btn{display:inline-block;width:70px;height:70px;line-height:70px;margin:5px;font-size:18px;font-weight:bold;background:#3498db;color:#fff;border-radius:15px;user-select:none;cursor:pointer;}";
  html += ".btn:active{background:#2980b9;transform:scale(0.95);}";
  html += ".btn-refresh{background:#2ecc71; color:#fff; padding:10px 15px; border:none; border-radius:5px; font-size:16px; cursor:pointer; margin-bottom:10px; font-weight:bold;}";
  html += ".btn-refresh:active{transform:scale(0.95);}";
  html += "h2, h3{margin-top:0;}";
  html += "</style></head><body>";
  
  html += "<h2>ROBOT DASHBOARD</h2>";
  
  // 網頁遙控器 
  html += "<div class='card'><h3>動態搖桿</h3>";
  
  // 手機：手指碰到時(ontouchstart)前進，移開時(ontouchend)自動煞車
  // 加上 JS 連發函式 startCmd() 與 stopCmd()
  html += "<div><div onmousedown='startCmd(\"F\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"F\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>前進</div></div>";
  html += "<div>";
  html += "<div onmousedown='startCmd(\"L\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"L\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>左轉</div> ";
  html += "<div onmousedown='stopCmd()' ontouchstart='stopCmd();event.preventDefault();' style='background:#e74c3c;' class='btn'>煞車</div> ";
  html += "<div onmousedown='startCmd(\"R\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"R\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>右轉</div>";
  html += "</div>";
  html += "<div><div onmousedown='startCmd(\"B\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"B\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>後退</div></div>";
  
  // 搖桿連發心跳 JS (Heartbeat)：按住時每 250ms 自動連發信號，確保不會被看門狗煞車
  html += "<script>";
  html += "let moveInterval;";
  html += "function startCmd(d){";
  html +=   "if(moveInterval) clearInterval(moveInterval);";
  html +=   "fetch('/cmd?dir='+d);";
  html +=   "moveInterval = setInterval(()=>{ fetch('/cmd?dir='+d); }, 250);";
  html += "}";
  html += "function stopCmd(){";
  html +=   "if(moveInterval){ clearInterval(moveInterval); moveInterval = null; }";
  html +=   "fetch('/cmd?dir=S');";
  html += "}";
  html += "</script>";
  html += "</div>";

  // 感測器區塊
  html += "<div class='card sensor'><h3>環境感測</h3>";
  html += "<div style='text-align:center;'><button class='btn-refresh' onclick='fetchSensors()'>取得環境值</button></div>";
  html += "溫度: <b id='val-temp'>" + String(currentData.temperature, 1) + " °C</b><br>";
  html += "濕度: <b id='val-hum'>" + String(currentData.humidity, 1) + " %</b><br>";
  html += "光照: <b id='val-light'>" + String(currentData.illuminance) + " %</b><br>";
  html += "雨水: <b id='val-rain'>" + String(currentData.rainDetection ? "<span style='color:#e74c3c;'>有雨</span>" : "<span style='color:#2ecc71;'>無雨</span>") + "</b>";
  html += "</div>";
  
  // GPS 區塊
  html += "<div class='card sensor'><h3>GPS 定位</h3>";
  html += "衛星數量: <b id='val-sat'>" + String(gps.satellites.value()) + "</b><br>";
  html += "<div id='val-gps'>";
  if (gps.location.isValid()) {
    html += "緯度: " + String(gps.location.lat(), 6) + "<br>";
    html += "經度: " + String(gps.location.lng(), 6) + "<br>";
  } else {
    html += "<span style='color:#e74c3c;'>尚未定位</span>";
  }
  html += "</div></div>";

  html += "<script>";
  // fetchSensors() 使用 AJAX 呼叫 /status。
  // 網頁不用重新整理，只更新數值所在的 DOM 元素。
  html += "function fetchSensors(){";
  html += "  fetch('/status').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('val-temp').innerText = d.temp.toFixed(1) + ' °C';";
  html += "    document.getElementById('val-hum').innerText = d.hum.toFixed(1) + ' %';";
  html += "    document.getElementById('val-light').innerText = d.light + ' %';";
  html += "    document.getElementById('val-rain').innerHTML = d.rain ? \"<span style='color:#e74c3c;'>有雨</span>\" : \"<span style='color:#2ecc71;'>無雨</span>\";";
  html += "    document.getElementById('val-sat').innerText = d.sat;";
  html += "    if(d.fix){ document.getElementById('val-gps').innerHTML = '緯度: '+d.lat+'<br>經度: '+d.lng+'<br>'; }";
  html += "    else { document.getElementById('val-gps').innerHTML = \"<span style='color:#e74c3c;'>尚未定位</span>\"; }";
  html += "  }).catch(e=>console.log(e));";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- 5.4 WiFi 連線與斷線處理 ---
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  // disconnect(true) 會清除目前連線狀態，讓後續 begin() 從乾淨狀態重新連線。
  WiFi.disconnect(true);

  // 在 FreeRTOS 任務中使用 vTaskDelay()，讓系統可以切換給其他任務執行。
  vTaskDelay(100 / portTICK_PERIOD_MS);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  // 給予手機熱點 20 秒鐘的配發 IP 時間
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    Serial.print("."); 
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 連線成功！ IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi 連線失敗，請檢查網路或是頻段(需2.4GHz)");
  }
}

// --- 5.5 硬體感測器讀取 ---
void readSensors() {
  // DHT11 讀取可能失敗，失敗時會得到 NaN。
  // 只有讀到有效數值時才更新 currentData，避免儀表板顯示無效資料。
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    currentData.temperature = t;
    currentData.humidity = h;
  }
  
  // 光照度：將 ADC 原始值線性換算為百分比 (0~4095 -> 0~100%)
  // GPIO 34 為類比輸入腳；analogRead() 在 ESP32 上預設約為 12 位元解析度。
  int lightRaw = analogRead(PIN_LIGHT);
  int lightPct = map(lightRaw, 0, 4095, 0, 100);  // 亮 -> 高
  lightPct = constrain(lightPct, 0, 100);
  currentData.illuminance = lightPct;

  // 雨水偵測：數位讀值 (X0100 MTARDRAIN 114)
  // 此類雨水模組常見為 Active Low：有水時輸出 LOW，乾燥時輸出 HIGH。
  int rainRaw = digitalRead(PIN_RAIN);
  currentData.rainDetection = (rainRaw == HIGH) ? 0 : 1;
}

// --- 5.6 ThingSpeak 封包上傳 ---
void uploadData() {
  Serial.println("Core 0: 上傳 ThingSpeak (馬達不卡頓!)");
  HTTPClient http;

  // 將多個感測值放進 ThingSpeak 不同欄位：
  // field1：雨水狀態
  // field2：溫度
  // field3：濕度
  // field4：光照百分比
  String url = String(thingSpeakServer) + "?api_key=" + String(writeAPIKey);
  url += "&field1=" + String(currentData.rainDetection);
  url += "&field2=" + String(currentData.temperature);
  url += "&field3=" + String(currentData.humidity);
  url += "&field4=" + String(currentData.illuminance);
  
  http.begin(url);
  int httpCode = http.GET();

  // ThingSpeak 成功寫入時，回應內容通常是 entry id，轉成整數會大於 0。
  if (httpCode > 0 && http.getString().toInt() > 0) {
    Serial.println("ThingSpeak Upload OK");
  } else {
    Serial.println("ThingSpeak Upload Fail");
  }
  http.end();
}

// --- 5.7 GPS status Web API ---
void handleStatus() {
  // 允許其他網頁來源讀取此 API，方便未來做外部儀表板或資料整合。
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // 手動組出 JSON 字串。
  // ESP32 記憶體有限，這裡避免額外引入大型 JSON 函式庫。
  String json = "{";
  json += "\"device_id\":\"" + String(device_id) + "\",";
  json += "\"sat\":" + String(gps.satellites.value()) + ",";

  // GPS 尚未定位時，TinyGPSPlus 會回報 location 無效。
  // 前端可根據 fix:true/false 決定顯示座標或「尚未定位」。
  if (gps.location.isValid()) {
    json += "\"lat\":" + String(gps.location.lat(), 6) + ",";
    json += "\"lng\":" + String(gps.location.lng(), 6) + ",";
    json += "\"fix\":true,";
  } else {
    json += "\"lat\":0,\"lng\":0,\"fix\":false,";
  }

  // 若 DHT11 尚未成功讀取，避免輸出 NaN 造成前端 JSON 解析問題。
  json += "\"temp\":" + (isnan(currentData.temperature) ? "0.0" : String(currentData.temperature, 1)) + ",";
  json += "\"hum\":" + (isnan(currentData.humidity) ? "0.0" : String(currentData.humidity, 1)) + ",";
  json += "\"light\":" + String(currentData.illuminance) + ",";
  json += "\"rain\":" + String(currentData.rainDetection);
  json += "}";
  server.send(200, "application/json", json);
}
