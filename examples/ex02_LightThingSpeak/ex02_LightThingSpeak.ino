/*
 * 練習 02 - 光敏電阻上傳 ThingSpeak
 *
 * 光敏電阻 AO 接 GPIO 34，讀值上傳到 ThingSpeak field1。
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <smhs_IoT_Robot.h>

const char* ssid = "your_ssid";
const char* password = "your_password";
const char* writeApiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

const int PIN_LIGHT = 34;
const unsigned long UPLOAD_INTERVAL = 20000;

stateLED led(SMHS_HW::WSLED, 1);
unsigned long lastUploadTime = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());
  led.fillColor(stateLED::GREEN);
}

void uploadLight(int value) {
  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=";
  url += writeApiKey;
  url += "&field1=";
  url += String(value);

  http.begin(url);
  int code = http.GET();
  http.end();

  Serial.print("Light: ");
  Serial.print(value);
  Serial.print("  ThingSpeak: ");
  Serial.println(code > 0 ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  led.begin();
  led.fillColor(stateLED::RED);
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    led.fillColor(stateLED::RED);
    connectWiFi();
  }

  if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
    lastUploadTime = millis();
    led.fillColor(stateLED::BLUE);
    uploadLight(analogRead(PIN_LIGHT));
    led.fillColor(stateLED::GREEN);
  }
}
