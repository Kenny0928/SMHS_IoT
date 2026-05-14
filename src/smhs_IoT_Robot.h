#ifndef SMHS_IOT_ROBOT_H
#define SMHS_IOT_ROBOT_H
#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // 引入底層 LED 驅動引擎

/*--------------------------------------------------------*/
// SMHS 硬體預設腳位定義 (可選載入)
/*--------------------------------------------------------*/
// 如果使用者在 #include "smhs_IoT_Robot.h" 之前 #define NO_SMHS_HW_PINS 則不會載入以下定義，避免名稱衝突。
#ifndef NO_SMHS_HW_PINS
namespace SMHS_HW {
  // DC 電機 (M1~M4)
  constexpr uint8_t M1A = 14; constexpr uint8_t M1B = 32;
  constexpr uint8_t M2A = 12; constexpr uint8_t M2B = 33;
  constexpr uint8_t M3A = 25; constexpr uint8_t M3B = 26;
  constexpr uint8_t M4A = 23; constexpr uint8_t M4B = 22;
  
  // 蜂鳴器
  constexpr uint8_t BUZZER = 15;
  // ws2812b全彩 / NeoPixel 指示燈
  constexpr uint8_t WSLED = 2;

  // 供電偵測
  constexpr uint8_t BATTERY = 36;
}
#endif


/*--------------------------------------------------------*/
// BUZZER 
/*--------------------------------------------------------*/
// 1. 改用 namespace 與 constexpr 節省記憶體，避免重複定義
namespace Pitch {
  constexpr uint16_t Do_3=131, Re_3=147, Mi_3=165, Fa_3=175, Sol_3=196, La_3=220, Si_3=247;
  constexpr uint16_t Do_4=262, Re_4=294, Mi_4=330, Fa_4=349, Sol_4=392, La_4=440, Si_4=494;
  constexpr uint16_t Do_5=523, Re_5=587, Mi_5=659, Fa_5=698, Sol_5=784, La_5=880, Si_5=988;
}

// 定義音符結構
struct Note {
  uint16_t freq;      // 頻率 (Hz)
  uint16_t duration;  // 持續時間 (毫秒)
};

/*** BUZZER ***/
class Buzzer {
  private:
    byte _pin;
    
    // 背景播放狀態機變數
    const Note* _currentMelody;
    uint16_t _melodyLength;
    uint16_t _noteIndex;
    unsigned long _lastNoteTime;
    bool _isPlaying;

  public:
    Buzzer(byte pin);
    void noTone();
    // 基礎發聲 (注意：這裡移除了 duration 的 delay，改為純發聲)
    void tone(uint16_t frequency, uint8_t volume = 127); 
	
	  void alarm(int freq = -1);
    
    // 背景播放引擎：負責在 loop 中推進音符
    void update();
    
    // 播放自訂旋律
    void playMelody(const Note* melody, uint16_t length);

    // 內建的系統豐富音效
    void soundBoot();      // 開機音效
    void soundConnect();   // 連線成功
    void soundError();     // 錯誤警告
    void soundNotify();    // 提醒音效
    void soundAttack();    // 攻擊/投石警報
};

/*--------------------------------------------------------*/
// stateLED (WS2812B全彩狀態指示燈) 
/*--------------------------------------------------------*/
class stateLED {
  private:
    uint8_t _pin;
    uint16_t _num_leds;
    Adafruit_NeoPixel _strip; // 組合 (Composition)：把 Adafruit 物件包在裡面

  public:
	// 定義狀態燈顏色
    typedef enum {
      OFF,       // 關閉 (黑)
      RED,       // 紅光 (常代表：攻擊、錯誤、低電量)
      GREEN,     // 綠光 (常代表：安全、待機、就緒)
      BLUE,      // 藍光 (常代表：藍牙已連接、運行中)
      YELLOW,    // 黃光 (常代表：警告、搜尋敵人、壓線邊緣)
      PURPLE,    // 紫光 (常代表：特殊模式、異常)
      CYAN,      // 青光/水藍光 (常代表：另一種自訂狀態)
      WHITE      // 白光 (常代表：照明、系統啟動)
    } Color;

    // 建構子：傳入腳位與 LED 數量
    stateLED(uint8_t pin, uint16_t num_leds);
    
    // 初始化函式 (要在主程式 setup 中呼叫)
    void begin();
	
	  void setBrightness(uint8_t brightness);
    void clear();
	
    // 單顆控制：設定單顆顏色或全亮
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
	  void setColor(uint16_t index, Color color);    
	
    // 一鍵切換機器人狀態燈效
    void fillColor(Color color);
	  void fill(uint8_t r, uint8_t g, uint8_t b);	
};

/*--------------------------------------------------------*/
// POWER 電壓讀取
/*--------------------------------------------------------*/
class Power {
  private:
    uint8_t _pin;
  public:
    Power(uint8_t pin);
    float read();
}; 

/*--------------------------------------------------------*/
// UltraSonic 超音波測距
/*--------------------------------------------------------*/
class HCSR04 {
  private:
    uint8_t _pin_echo;
    uint8_t _pin_trig;

  public:
    typedef enum { USC_FRONT, USC_LEFT, USC_RIGHT } USC_DIR;

    HCSR04(uint8_t pin_echo, uint8_t pin_trig);
    bool ObjSeeking(uint8_t thresh);
    float ObjDistance();
    unsigned long probing();
};  

/*****------- IR3CH (Non-blocking) -------*****/
class IR3CH {
  private:
    uint8_t _pin_L;
    uint8_t _pin_M;
    uint8_t _pin_R;
    
    // 背景狀態機與計時變數
    unsigned long _lastUpdate;
    unsigned long _lastDebugPrint;
    byte _currentState;

  public:
    // 新增：定義循線/邊界的物理顏色
    typedef enum {
      BLACK_LINE, // 數值 > TH 觸發 (吸光，反射弱)
      WHITE_LINE  // 數值 < TH 觸發 (反光，反射強)
    } LineType;
	
    IR3CH(uint8_t pin_L, uint8_t pin_M, uint8_t pin_R);
    
    // 必須在主迴圈呼叫，負責定期在背景更新感測器數值
    void update(uint16_t TH, LineType lineType = BLACK_LINE, bool DEBUG = false);
	
	  //一次性讀取3通道數值並轉換為byte數值, traditional way
	  byte read3CH(uint16_t TH, LineType lineType = BLACK_LINE, bool DEBUG = false);
    
    // 瞬間取得最新的循跡狀態 (000 ~ 111)，不浪費時間
    byte getState();
    
    // 獨立取得單一感測器的類比原始數值 (供進階演算法使用)
    uint16_t getValL();
    uint16_t getValM();
    uint16_t getValR();
};


/*****------- MOTOR -------*****/
class Motor {
  private:
    uint16_t _m_frequency = 1000;
    uint8_t _m_resolution = 10;
    uint8_t _pin_a;
    uint8_t _pin_b;
    // ESP32 v3.0.0 不再需要手動管理 channel

  public:
    typedef enum { STOP, CW, CCW } Motor_Action;

    Motor(uint8_t pin_a, uint8_t pin_b); // 移除 channel 參數
    void act(Motor_Action dir, uint16_t speed); // 改用 Enum 強型別
    void stop();
};

/*****------- GoSUMO -------*****/
class GoSUMO {
  private:
    // 宣告四個馬達的指標，用來接收外部傳入的實體馬達
    Motor* _m1;
    Motor* _m2;
    Motor* _m3;
    Motor* _m4;

  public:
    typedef enum {
	    // 基礎運動 (Basic Movement)
      FORWARD,     // 前進
      BACKWARD,    // 後退
      TURN_LEFT,   // 原地左轉 (原 GO_LEFT)
      TURN_RIGHT,  // 原地右轉 (原 GO_RIGHT)

      // 橫移運動 (Strafing - 麥克納姆輪特性)
      STRAFE_LEFT,  // 左橫移 (原 L_TVL)
      STRAFE_RIGHT, // 右橫移 (原 R_TVL)

      // 斜向運動 (Diagonal)
      DIAG_FL,     // 前左斜向 (Forward-Left, 原 LF_TVL)
      DIAG_FR,     // 前右斜向 (Forward-Right, 原 RF_TVL)
      DIAG_BL,     // 後左斜向 (Backward-Left, 原 LB_TVL)
      DIAG_BR      // 後右斜向 (Backward-Right, 原 RB_TVL)
    } Motion;

    // 建構子：要求傳入四顆馬達的記憶體位址
    GoSUMO(Motor* m1, Motor* m2, Motor* m3, Motor* m4);
    
    void act(Motion motion, uint16_t speed);
    void linetracking(Motion motion, uint16_t speed, uint16_t wheel_difference);
    void stop();   
};

#endif
