/*
 * 練習 02 - 光敏電阻上傳 ThingSpeak
 *
 * 光敏電阻 AO 接 GPIO 34，讀值上傳到 ThingSpeak field1。
 *
 * 學習目標：
 * - 學習 ESP32 連接 WiFi
 * - 學習用 HTTP GET 將感測器資料送到雲端平台
 * - 認識 ThingSpeak 的 Write API Key 與 field 欄位
 * - 學習用 millis() 做非阻斷式定時上傳
 *
 * 使用前請先修改：
 * - ssid：你的 WiFi 名稱
 * - password：你的 WiFi 密碼
 * - writeApiKey：ThingSpeak Channel 的 Write API Key
 *
 * ThingSpeak 限制：
 * - 免費頻道通常建議至少間隔 15 秒再上傳一次。
 * - 本範例設定為每 20 秒上傳一次，避免太頻繁被拒收。
 */

// WiFi.h：ESP32 內建 WiFi 連線功能。
#include <WiFi.h>
// HTTPClient.h：用來發送 HTTP GET / POST 請求。
#include <HTTPClient.h>
// smhs_IoT_Robot.h：用來控制板上的 WS2812B 狀態燈。
#include <smhs_IoT_Robot.h>

// --- 網路與 ThingSpeak 設定 ---
// 請把字串內容改成自己的 WiFi 與 ThingSpeak 資料。
const char* ssid = "your_ssid";
const char* password = "your_password";
const char* writeApiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

// --- 硬體與時間設定 ---
// 光敏電阻的 AO 腳接 GPIO 34。
const int PIN_LIGHT = 34;

// 上傳間隔，單位是毫秒。20000 ms = 20 秒。
const unsigned long UPLOAD_INTERVAL = 20000;

// --- 建立物件與全域變數 ---
// 狀態燈：紅色代表尚未連線或連線中，綠色代表 WiFi 正常，藍色代表正在上傳。
stateLED led(SMHS_HW::WSLED, 1);

// 記錄上一次上傳的時間點，用來和 millis() 比較。
unsigned long lastUploadTime = 0;

// 連接 WiFi 的副程式。
// 將 WiFi 連線流程包成函式，可以在 setup() 與斷線重連時重複使用。
void connectWiFi() {
  // 設為 STA 模式，表示 ESP32 要連到既有的 WiFi 分享器或手機熱點。
  WiFi.mode(WIFI_STA);

  // 開始連接指定的 WiFi。
  WiFi.begin(ssid, password);

  // 只要還沒連上，就每 0.5 秒印出一個點，讓使用者知道程式仍在等待。
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // 連線成功後印出 ESP32 取得的 IP 位址。
  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());

  // 綠燈表示網路已連線。
  led.fillColor(stateLED::GREEN);
}

// 將光敏電阻讀值上傳到 ThingSpeak。
// value 會被放到 field1，ThingSpeak 後台可用圖表觀察變化。
void uploadLight(int value) {
  // HTTPClient 是 Arduino ESP32 提供的 HTTP 請求物件。
  HTTPClient http;

  // 組合 ThingSpeak 更新網址：
  // http://api.thingspeak.com/update?api_key=金鑰&field1=數值
  String url = "http://api.thingspeak.com/update?api_key=";
  url += writeApiKey;
  url += "&field1=";
  url += String(value);

  // 指定要連線的 URL。
  http.begin(url);

  // 發送 HTTP GET 請求，回傳值 code > 0 代表有收到 HTTP 層級回應。
  int code = http.GET();

  // 釋放 HTTP 連線資源，避免長時間執行後資源不足。
  http.end();

  // 在 Serial Monitor 顯示本次上傳的感測器數值與結果。
  Serial.print("Light: ");
  Serial.print(value);
  Serial.print("  ThingSpeak: ");
  Serial.println(code > 0 ? "OK" : "FAIL");
}

void setup() {
  // 啟動序列埠，方便查看 WiFi 連線與上傳狀態。
  Serial.begin(115200);

  // 啟動狀態燈。
  led.begin();

  // 紅燈代表正在開機或等待 WiFi。
  led.fillColor(stateLED::RED);

  // 連上 WiFi 後才進入主迴圈。
  connectWiFi();
}

void loop() {
  // 如果 WiFi 斷線，亮紅燈並重新連線。
  if (WiFi.status() != WL_CONNECTED) {
    led.fillColor(stateLED::RED);
    connectWiFi();
  }

  // millis() 會回傳 ESP32 開機後經過的毫秒數。
  // 這種寫法不會像 delay(20000) 一樣讓整個程式停住。
  if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
    // 先記錄本次上傳時間，下一次要再等 UPLOAD_INTERVAL。
    lastUploadTime = millis();

    // 藍燈表示正在進行雲端上傳。
    led.fillColor(stateLED::BLUE);

    // 讀取光敏電阻並立即上傳。
    uploadLight(analogRead(PIN_LIGHT));

    // 上傳完成後回到綠燈，表示 WiFi 正常待機。
    led.fillColor(stateLED::GREEN);
  }
}
