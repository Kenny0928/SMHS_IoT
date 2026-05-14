# smhs_IoT_Robot

ESP32 IoT 機器人課程用函式庫與範例程式。

## 安裝方式

1. 在 GitHub 下載此專案 ZIP。
2. 開啟 Arduino IDE。
3. 選擇「草稿碼 > 匯入程式庫 > 加入 .ZIP 程式庫」。
4. 選擇下載的 ZIP 檔。
5. 安裝完成後，可從「檔案 > 範例 > smhs_IoT_Robot」開啟範例。

## 專案結構

```text
smhs_IoT_Robot/
├── src/
│   ├── smhs_IoT_Robot.h
│   └── smhs_IoT_Robot.cpp
├── examples/
│   ├── ex01_DHT11/
│   ├── ex02_Rain/
│   ├── ex03_Light/
│   ├── ex04_RobotControl/
│   └── smhs_IoT_Robot/
├── library.properties
├── README.md
└── smhs_IoT_Robot_guide.html
```

函式庫本體只保留一份 `src/smhs_IoT_Robot.h` 與 `src/smhs_IoT_Robot.cpp`。範例程式使用：

```cpp
#include <smhs_IoT_Robot.h>
```

## 範例

- `ex01_DHT11`: DHT11 溫濕度感測器與網頁儀表板
- `ex02_Rain`: 雨水感測器數位輸入與網頁儀表板
- `ex03_Light`: 光照感測器類比輸入與網頁儀表板
- `ex04_RobotControl`: WiFi 網頁遙控車體
- `smhs_IoT_Robot`: 整合版進階範例

## 依賴函式庫

- ESP32 board package
- Adafruit NeoPixel
- DHT sensor library
- TinyGPSPlus

`WiFi.h`、`WebServer.h`、`HTTPClient.h` 由 ESP32 board package 提供。
