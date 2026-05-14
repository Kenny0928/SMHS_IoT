#include "smhs_IoT_Robot.h"

/*--------------------------------------------------------*/
// BUZZER 
/*--------------------------------------------------------*/
Buzzer::Buzzer(byte pin) : _pin(pin), _isPlaying(false) {
  // 不再預設 pinMode，交給 ledcAttach 處理
}

void Buzzer::noTone() {
  ledcWrite(_pin, 0);
  ledcDetach(_pin);
}

void Buzzer::tone(uint16_t frequency, uint8_t volume) {
  if (frequency == 0) {
    noTone(); // 頻率為 0 視為休止符
    return;
  }
  // 【關鍵修正】先將腳位卸載，才能強制 ESP32 v3 核心更新頻率
  ledcDetach(_pin);
  
  ledcAttach(_pin, frequency, 8); 
  ledcWrite(_pin, volume); 
}

//阻塞式聲響控制 -- 簡單警報 ====================================
void Buzzer::alarm(int freq){
  int _FREQ;
  if (freq == -1) {  _FREQ = 800; } else { _FREQ = freq;   }
  tone(_FREQ, 50);
  delay(200);
  noTone(); 
}

//非阻塞式聲響控制 ==============================================
void Buzzer::update() {
  // 如果沒有在播放音樂，直接跳出，完全不佔用資源
  if (!_isPlaying) return;

  unsigned long elapsedTime = millis() - _lastNoteTime;
  uint16_t currentDuration = _currentMelody[_noteIndex].duration;

  // 自動斷音機制
  // 在每個音符結束前 20 毫秒，提前將音量設為 0 (靜音)
  // 這能切開兩個連續相同的音符，產生清晰的節奏感
  if (currentDuration > 20 && elapsedTime >= currentDuration - 20) {
    ledcWrite(_pin, 0); 
  }

  // 檢查當前音符的總時間是否結束
  if (elapsedTime >= currentDuration) {
    _noteIndex++; // 準備播放下一個音

    // 判斷是否播完整首旋律
    if (_noteIndex >= _melodyLength) {
      _isPlaying = false;
      noTone();
    } else {
      // 播放下一個音符，並記錄開始時間
      tone(_currentMelody[_noteIndex].freq);
      _lastNoteTime = millis();
    }
  } 

}

void Buzzer::playMelody(const Note* melody, uint16_t length) {
  _currentMelody = melody;
  _melodyLength = length;
  _noteIndex = 0;
  _lastNoteTime = millis();
  _isPlaying = true;
  
  // 立即播放第一個音
  tone(_currentMelody[0].freq);
}

// ================= 內建豐富音效庫 =================
void Buzzer::soundBoot() {
  // 開機：快速上升琶音 (瑪利歐吃蘑菇的感覺)
  static const Note melody[] = {
    {Pitch::Do_4, 100}, {Pitch::Mi_4, 100}, {Pitch::Sol_4, 100}, {Pitch::Do_5, 200}
  };
  playMelody(melody, 4);
}

void Buzzer::soundConnect() {
  // 連線：輕快雙響 (休止符用頻率 0 表示)
  static const Note melody[] = {
    {Pitch::Mi_5, 80}, {0, 50}, {Pitch::Sol_5, 120}
  };
  playMelody(melody, 3);
}

void Buzzer::soundError() {
  // 錯誤：沉悶的低音
  static const Note melody[] = {
    {Pitch::Do_3, 300}, {0, 50}, {Pitch::Do_3, 300}
  };
  playMelody(melody, 3);
}

void Buzzer::soundNotify() {
  // 攻擊：急促的高頻警告音
  static const Note melody[] = {
    {1000, 50}, {0, 50}, {1000, 50}, {0, 50}, {1000, 50} 
  };
  playMelody(melody, 5);
}

void Buzzer::soundAttack() {
  // 攻擊：急促的高頻警告音
  static const Note melody[] = {
    // 登、登、登
    {Pitch::Sol_4, 300}, {Pitch::Sol_4, 300}, {Pitch::Sol_4, 300}, 
    // (登~登)、登—— (準備撞擊！)
    {311, 200}, {466, 100}, {Pitch::Sol_4, 600}
  };
  playMelody(melody, 9);
}

/*--------------------------------------------------------*/
// stateLED (WS2812B全彩狀態指示燈) 
/*--------------------------------------------------------*/
// 初始化列表：設定 Adafruit_NeoPixel 物件 (預設為 GRB 格式與 800KHz)
stateLED::stateLED(uint8_t pin, uint16_t num_leds) 
  : _pin(pin), _num_leds(num_leds), _strip(num_leds, pin, NEO_GRB + NEO_KHZ800) {
}

void stateLED::begin() {
  _strip.begin();
  _strip.show(); // 關閉所有 LED
  _strip.setBrightness(100); 
}

void stateLED::setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
  _strip.setPixelColor(index, _strip.Color(r, g, b));
  _strip.show();
}

void stateLED::fill(uint8_t r, uint8_t g, uint8_t b) {
  _strip.fill(_strip.Color(r, g, b), 0, _num_leds);
  _strip.show();
}

void stateLED::clear() {
  _strip.clear();
  _strip.show();
}

void stateLED::setBrightness(uint8_t brightness) {
  _strip.setBrightness(brightness);
  _strip.show();
}

// 根據 Enum 一鍵設定燈光顏色
void stateLED::fillColor(Color color) {
  switch (color) {
    case OFF:    
      fill(0, 0, 0); 
      break;
    case RED:    
      fill(255, 0, 0); 
      break;
    case GREEN:  
      fill(0, 255, 0); 
      break;
    case BLUE:   
      fill(0, 0, 255); 
      break;
    case YELLOW: 
      fill(255, 150, 0); 
      break;
    case PURPLE: 
      fill(128, 0, 128); 
      break;
    case CYAN:   
      fill(0, 255, 255); 
      break;
    case WHITE:  
      fill(200, 200, 200); 
      break;
  }
}

// 根據 Enum 設定單顆LED 顏色
void stateLED::setColor(uint16_t index, Color color) {
  if (index >= _num_leds) return;   
  switch (color) {
    case OFF:    
      setPixel(index, 0, 0, 0); 
      break;
    case RED:    
      setPixel(index, 255, 0, 0); 
      break;
    case GREEN:  
      setPixel(index, 0, 255, 0); 
      break;
    case BLUE:   
      setPixel(index, 0, 0, 255); 
      break;
    case YELLOW: 
      setPixel(index, 255, 150, 0); 
      break;
    case PURPLE: 
      setPixel(index, 128, 0, 128); 
      break;
    case CYAN:   
      setPixel(index, 0, 255, 255); 
      break;
    case WHITE:  
      setPixel(index, 100, 100, 100); 
      break;
  }
}

/*--------------------------------------------------------*/
// POWER 電壓讀取
/*--------------------------------------------------------*/
Power::Power(uint8_t pin) : _pin(pin) {
  pinMode(_pin, INPUT);
  analogReadResolution(12);  
  analogSetAttenuation(ADC_11db);
}

float Power::read() {
  float _fVal = (analogRead(_pin) * 0.00088) / 0.2546;
  return _fVal;
}

/*--------------------------------------------------------*/
// UltraSonic 超音波測距
/*--------------------------------------------------------*/
HCSR04::HCSR04(uint8_t pin_echo, uint8_t pin_trig) : _pin_echo(pin_echo), _pin_trig(pin_trig) {
  pinMode(_pin_trig, OUTPUT); 
  digitalWrite(_pin_trig, LOW); 
  pinMode(_pin_echo, INPUT);   
}

unsigned long HCSR04::probing() {
  digitalWrite(_pin_trig, LOW);
  delayMicroseconds(2);
  digitalWrite(_pin_trig, HIGH);
  delayMicroseconds(10);       
  digitalWrite(_pin_trig, LOW);
  return pulseIn(_pin_echo, HIGH, 20000); 
}

float HCSR04::ObjDistance() {
  unsigned long p = probing();
  if(p == 0) return 999.0;     
  return p / 58.0;
}

bool HCSR04::ObjSeeking(uint8_t thresh) {
  unsigned long p = probing();
  if(p == 0) return false;          
  return (p < (thresh * 58));
}

/*--------------------------------------------------------*/
// IR3CH (Non-blocking Implementation)
/*--------------------------------------------------------*/
IR3CH::IR3CH(uint8_t pin_L, uint8_t pin_M, uint8_t pin_R) 
  : _pin_L(pin_L), _pin_M(pin_M), _pin_R(pin_R), 
    _lastUpdate(0), _lastDebugPrint(0), _currentState(0) {
  pinMode(_pin_L, INPUT);
  pinMode(_pin_M, INPUT);
  pinMode(_pin_R, INPUT);
}

byte IR3CH::read3CH(uint16_t TH, LineType lineType, bool DEBUG) {
    byte tempState = 0;
    uint16_t valR = analogRead(_pin_R);
    uint16_t valM = analogRead(_pin_M);
    uint16_t valL = analogRead(_pin_L);

    if (lineType == BLACK_LINE) {
      if(valR > TH) tempState |= B001;
      if(valM > TH) tempState |= B010;
      if(valL > TH) tempState |= B100;
    } 
    else if (lineType == WHITE_LINE) {
      if(valR < TH) tempState |= B001;
      if(valM < TH) tempState |= B010;
      if(valL < TH) tempState |= B100;
    }
    
    _currentState = tempState;

    if (DEBUG) {
      Serial.print("IR_L: "); Serial.print(valL);
      Serial.print(" | IR_M: "); Serial.print(valM);
      Serial.print(" | IR_R: "); Serial.print(valR);
      Serial.print(" => State: "); Serial.println(_currentState, BIN);
    }
	return _currentState;
}

void IR3CH::update(uint16_t TH, LineType lineType, bool DEBUG) {
  unsigned long currentMillis = millis();

  // 每 10 毫秒讀取一次 ADC (100Hz 採樣頻率)
  if (currentMillis - _lastUpdate >= 10) {
    _lastUpdate = currentMillis;
    
    byte tempState = 0;
    uint16_t valR = analogRead(_pin_R);
    uint16_t valM = analogRead(_pin_M);
    uint16_t valL = analogRead(_pin_L);

    //由高至低位元 L, M, R
    if (lineType == BLACK_LINE) {
      if(valR > TH) tempState |= B001;
      if(valM > TH) tempState |= B010;
      if(valL > TH) tempState |= B100;
    } 
    else if (lineType == WHITE_LINE) {
      if(valR < TH) tempState |= B001;
      if(valM < TH) tempState |= B010;
      if(valL < TH) tempState |= B100;
    }
    
    _currentState = tempState; 

    // 每 500 毫秒印出一次
    if (DEBUG && (currentMillis - _lastDebugPrint >= 500)) {
      _lastDebugPrint = currentMillis;
      Serial.print("IR_L: "); Serial.print(valL);
      Serial.print(" | IR_M: "); Serial.print(valM);
      Serial.print(" | IR_R: "); Serial.print(valR);
      Serial.print(" => State: "); Serial.println(_currentState, BIN);
    }
  }
}

byte IR3CH::getState() {
  return _currentState;
}

uint16_t IR3CH::getValL() { return analogRead(_pin_L); }
uint16_t IR3CH::getValM() { return analogRead(_pin_M); }
uint16_t IR3CH::getValR() { return analogRead(_pin_R); }


/*--------------------------------------------------------*/
// GoSUMO
/*--------------------------------------------------------*/
// 將外部傳入的指標，綁定到內部的變數
GoSUMO::GoSUMO(Motor* m1, Motor* m2, Motor* m3, Motor* m4) {
  _m1 = m1;
  _m2 = m2;
  _m3 = m3;
  _m4 = m4;
}

void GoSUMO::act(Motion motion, uint16_t speed) {
  switch (motion) {
    case FORWARD:
      _m1->act(Motor::CCW, speed); _m3->act(Motor::CCW, speed);
      _m2->act(Motor::CW, speed);  _m4->act(Motor::CW, speed);
      break;
    case BACKWARD:
      _m1->act(Motor::CW, speed);  _m3->act(Motor::CW, speed);
      _m2->act(Motor::CCW, speed); _m4->act(Motor::CCW, speed);
      break;
    case TURN_LEFT:
      _m1->act(Motor::CW, speed);  _m3->act(Motor::CW, speed);
      _m2->act(Motor::CW, speed);  _m4->act(Motor::CW, speed);
      break;
    case TURN_RIGHT:
      _m1->act(Motor::CCW, speed); _m3->act(Motor::CCW, speed);
      _m2->act(Motor::CCW, speed); _m4->act(Motor::CCW, speed);
      break;
    case STRAFE_RIGHT: // 右橫移
      _m1->act(Motor::CW, speed);  _m3->act(Motor::CCW, speed);
      _m2->act(Motor::CW, speed);  _m4->act(Motor::CCW, speed);
      break;
    case STRAFE_LEFT: // 左橫移
      _m1->act(Motor::CCW, speed); _m3->act(Motor::CW, speed);
      _m2->act(Motor::CCW, speed); _m4->act(Motor::CW, speed);
      break;  
    case DIAG_FR:  // 前右斜向 
      _m1->stop();                 _m3->act(Motor::CCW, speed);
      _m2->act(Motor::CW, speed);  _m4->stop();
      break;
    case DIAG_BR:  // 後右斜向 
      _m1->act(Motor::CW, speed);  _m3->stop();
      _m2->stop();                 _m4->act(Motor::CCW, speed);
      break;
    case DIAG_FL:  // 前左斜向 
      _m1->act(Motor::CCW, speed); _m3->stop();
      _m2->stop();                 _m4->act(Motor::CW, speed);
      break;
    case DIAG_BL:  // 後左斜向 
      _m1->stop();                 _m3->act(Motor::CW, speed);
      _m2->act(Motor::CCW, speed); _m4->stop();
      break;
  }
}

void GoSUMO::stop() {
  _m1->stop();
  _m2->stop();
  _m3->stop();
  _m4->stop();
}

void GoSUMO::linetracking(Motion motion, uint16_t speed, uint16_t wheel_difference) {
  if (motion == FORWARD) {
    _m1->act(Motor::CCW, speed); _m3->act(Motor::CCW, speed);
    _m2->act(Motor::CW, speed);  _m4->act(Motor::CW, speed);
  }
  else if (motion == BACKWARD) {
    _m1->act(Motor::CW, speed);  _m3->act(Motor::CW, speed);
    _m2->act(Motor::CCW, speed); _m4->act(Motor::CCW, speed);
  }
  else if (motion == TURN_LEFT) {
    _m1->act(Motor::CCW, speed - wheel_difference); _m3->act(Motor::CCW, speed - wheel_difference);
    _m2->act(Motor::CW, speed);                     _m4->act(Motor::CW, speed);
  }
  else if (motion == TURN_RIGHT) {
    _m1->act(Motor::CCW, speed);                    _m3->act(Motor::CCW, speed);
    _m2->act(Motor::CW, speed - wheel_difference);  _m4->act(Motor::CW, speed - wheel_difference);
  }
}

/*** MOTOR ***/ //-------------------------------------------------------------------
Motor::Motor(uint8_t pin_a, uint8_t pin_b) : _pin_a(pin_a), _pin_b(pin_b) {
  ledcAttach(_pin_a, _m_frequency, _m_resolution);
  ledcAttach(_pin_b, _m_frequency, _m_resolution);
}

void Motor::act(Motor_Action dir, uint16_t speed) {
  if (dir == CW) { ledcWrite(_pin_a, speed); ledcWrite(_pin_b, 0); }
  else if (dir == CCW) { ledcWrite(_pin_a, 0); ledcWrite(_pin_b, speed); }
}

void Motor::stop() { 
  ledcWrite(_pin_a, 0); ledcWrite(_pin_b, 0); 
}
