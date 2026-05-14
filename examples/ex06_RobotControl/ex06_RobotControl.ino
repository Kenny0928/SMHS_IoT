/*
 * 練習 06 - 網頁遙控車體控制 (純車體版)
 *
 * 學習目標：
 * - 認識 GoSUMO 四輪麥克納姆輪車的控制方式
 * - 了解 WiFi 網頁搖桿如何透過 AJAX 發送即時指令
 * - 認識「看門狗 (Watchdog)」機制：防止斷線後車子持續行駛
 * - 學習使用 WS2812B 全彩 LED 顯示系統狀態
 *
 * 操作方式：
 * 1. 修改下方 ssid / password 為你的 WiFi 帳號密碼
 * 2. 燒錄後打開 Serial 監控視窗，看到 IP 位址
 * 3. 手機或電腦瀏覽器輸入該 IP，開啟遙控介面
 * 4. 按住方向按鈕即可控制車子移動，放開自動煞車
 *
 * 看門狗機制說明：
 *   網頁搖桿每 250ms 送出一次「心跳」指令。
 *   若車體連續 500ms 沒收到心跳 (例如 WiFi 斷線)，
 *   看門狗會強制煞車，防止失控。
 */

// ==================== 1. 標頭檔區 ====================
#include <WiFi.h>
#include <WebServer.h>
#include <smhs_IoT_Robot.h>


// ==================== 2. 宣告區 ====================
// --- 網路設定 (請修改成你的 WiFi 名稱與密碼) ---
const char* ssid     = "your_ssid";
const char* password = "your_password";

// --- 小車速度設定 (0 ~ 1023) ---
// 數值越大速度越快，建議從 500 左右開始調整
int robotSpeed = 570;

// --- 看門狗計時器 ---
// volatile 宣告：告知編譯器此變數可能被不同任務同時修改，不可快取
volatile unsigned long lastCmdTime = 0;
const unsigned long CMD_TIMEOUT = 500;  // 500ms 無心跳即觸發看門狗煞車
volatile bool isMoving = false;

// --- 建立硬體物件 ---
Motor m1(SMHS_HW::M1A, SMHS_HW::M1B);
Motor m2(SMHS_HW::M2A, SMHS_HW::M2B);
Motor m3(SMHS_HW::M3A, SMHS_HW::M3B);
Motor m4(SMHS_HW::M4A, SMHS_HW::M4B);
GoSUMO robot(&m1, &m2, &m3, &m4);

WebServer server(80);
stateLED  robotLED(SMHS_HW::WSLED, 1);

// --- 副程式預先宣告 ---
void connectWiFi();
void handleDashboard();
void handleCommand();


// ==================== 3. 初始化區 ====================
void setup() {
  Serial.begin(115200);

  robot.stop();  // 確保開機時馬達靜止

  robotLED.begin();
  robotLED.fillColor(stateLED::RED);  // 開機亮紅燈，等待 WiFi 連線

  Serial.println("\n練習 06：網頁車體控制啟動中...");

  connectWiFi();

  server.on("/",    handleDashboard);  // 主頁：遙控介面
  server.on("/cmd", handleCommand);    // 指令端點：接收方向指令
  server.begin();

  Serial.println("Web Server 已啟動！");
  Serial.println("請在瀏覽器輸入上方 IP 位址開啟遙控介面\n");
}


// ==================== 4. 主程式區 ====================
void loop() {
  unsigned long currentTime = millis();

  // --- 看門狗自動煞車 ---
  // 若車子正在移動，但超過 500ms 沒收到任何指令，強制煞車！
  if (isMoving && (currentTime - lastCmdTime > CMD_TIMEOUT)) {
    robot.stop();
    isMoving = false;
    Serial.println("⚠️  看門狗觸發：超時自動煞車！");
  }

  // 持續監聽網頁請求
  server.handleClient();

  delay(10);  // 短暫讓出 CPU 時間，避免空轉過熱
}


// ==================== 5. 副程式區 ====================

// --- 5.1 網頁指令處理 ---
// 網頁搖桿按下時，瀏覽器會發送類似 /cmd?dir=F 的請求
void handleCommand() {
  if (server.hasArg("dir")) {
    String dir = server.arg("dir");

    // 每次收到任何指令，立刻更新看門狗的最後心跳時間
    lastCmdTime = millis();

    // 根據方向指令驅動馬達
    if      (dir == "F") { robot.act(GoSUMO::FORWARD,    robotSpeed); isMoving = true;  }
    else if (dir == "B") { robot.act(GoSUMO::BACKWARD,   robotSpeed); isMoving = true;  }
    else if (dir == "L") { robot.act(GoSUMO::TURN_LEFT,  robotSpeed); isMoving = true;  }
    else if (dir == "R") { robot.act(GoSUMO::TURN_RIGHT, robotSpeed); isMoving = true;  }
    else if (dir == "S") { robot.stop();                               isMoving = false; }

    Serial.print("收到指令：");
    Serial.println(dir);
  }

  // 回應 200 OK，AJAX 不重新載入頁面
  server.send(200, "text/plain", "OK");
}

// --- 5.2 網頁遙控介面 ---
void handleDashboard() {
  String html = "<!doctype html><html><head><meta charset='utf-8'>";
  // user-scalable=no：禁止手機縮放，避免誤觸時縮放畫面干擾操作
  html += "<meta name='viewport' content='width=device-width, user-scalable=no, initial-scale=1'>";
  html += "<title>練習06 - 車體控制</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;margin:10px;text-align:center;background:#2c3e50;color:#ecf0f1;touch-action:manipulation;}";
  html += ".card{background:#34495e;border-radius:10px;padding:15px;margin:10px auto;max-width:320px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  // user-select:none：按鈕按住時不會反白選取文字
  html += ".btn{display:inline-block;width:80px;height:80px;line-height:80px;margin:6px;font-size:20px;font-weight:bold;background:#3498db;color:#fff;border-radius:15px;user-select:none;cursor:pointer;}";
  html += ".btn:active{background:#2980b9;transform:scale(0.95);}";
  html += ".btn-stop{background:#e74c3c;}";
  html += ".btn-stop:active{background:#c0392b;}";
  html += ".status{font-size:14px;color:#95a5a6;margin-top:10px;}";
  html += "h1{margin-bottom:5px;}";
  html += ".subtitle{color:#95a5a6;font-size:14px;margin-bottom:15px;}";
  html += "</style></head><body>";

  html += "<h1>車體遙控</h1>";
  html += "<p class='subtitle'>練習 06 - smhs_IoT_Robot</p>";

  html += "<div class='card'>";
  html += "<h3 style='margin-top:0;'>方向搖桿</h3>";

  // 搖桿按鈕：
  //   onmousedown / ontouchstart → 按下時開始連發心跳
  //   onmouseup  / ontouchend   → 放開時發送煞車指令
  //   event.preventDefault()   → 防止手機觸控時觸發額外的 mouse 事件
  html += "<div>";
  html += "<div onmousedown='startCmd(\"F\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"F\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>前進</div>";
  html += "</div>";

  html += "<div>";
  html += "<div onmousedown='startCmd(\"L\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"L\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>左轉</div> ";
  html += "<div onmousedown='stopCmd()' ontouchstart='stopCmd();event.preventDefault();' class='btn btn-stop'>煞車</div> ";
  html += "<div onmousedown='startCmd(\"R\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"R\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>右轉</div>";
  html += "</div>";

  html += "<div>";
  html += "<div onmousedown='startCmd(\"B\")' onmouseup='stopCmd()' ontouchstart='startCmd(\"B\");event.preventDefault();' ontouchend='stopCmd();event.preventDefault();' class='btn'>後退</div>";
  html += "</div>";

  html += "<p class='status'>按住方向鍵移動，放開自動煞車</p>";
  html += "</div>";

  // 搖桿心跳 JavaScript
  // 按住按鈕時每 250ms 自動重送一次指令，確保看門狗不會觸發煞車
  html += "<script>";
  html += "let moveInterval;";
  // startCmd：開始連發心跳
  html += "function startCmd(d){";
  html +=   "if(moveInterval) clearInterval(moveInterval);";
  html +=   "fetch('/cmd?dir='+d);";  // 立即發送第一次
  html +=   "moveInterval = setInterval(()=>{ fetch('/cmd?dir='+d); }, 250);";  // 之後每 250ms 發送
  html += "}";
  // stopCmd：停止心跳並發送煞車
  html += "function stopCmd(){";
  html +=   "if(moveInterval){ clearInterval(moveInterval); moveInterval = null; }";
  html +=   "fetch('/cmd?dir=S');";
  html += "}";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- 5.3 WiFi 連線 ---
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
