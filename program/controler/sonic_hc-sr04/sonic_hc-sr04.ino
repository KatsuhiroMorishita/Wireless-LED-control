/* program name: sonic_hc-sr-4                                    */
/* author:  Izumi Shinobu, Katsuhiro MORISHITA                        */
/* purpose: 三井ホールで超音波センサを使った展示の操作を行う。                */
/* create:  2015-05-04                                                */
/* license:  MIT                                                      */
/* memo: Arduino IDE 1.6以上でないと、コンパイルでエラーがでる。             */
#include <AltSoftSerial.h>

#define PLATFORM_IS_UNO
//#define PLATFORM_IS_MEGA

// com
#ifdef PLATFORM_IS_UNO
AltSoftSerial _xbee_serial(8,9); // port is fixed on 8 & 9 at UNO.
Stream* xbee_serial = &_xbee_serial;
#endif
#ifdef PLATFORM_IS_MEGA
Stream* xbee_serial = &Serial1;
#endif

// protcol
const char header = 0x7f;    // 多数のモジュールを一つのコマンドで操作する（ノード毎のグラデーションがない状況に適している）
const char header_v2 = 0x3f; // 個々のモジュールに発光色を4byteで伝える
const char header_v3 = 0x4f; // 内部時計のリセット（同期用）
const char header_v4 = 0x5f; // オートモードへ切り替え
const char header_v5 = 0x6f; // 遠隔操作モードへ切り替え

// LED setup
const float briteness_scale = 0.5f;
long color_cycle_width = 120000l;        // 半周期にかける時間 ms
long brightness_cycle = 30000l;
long color_phase = 0l;
long brightness_phase = 0l;

// sonic
int Trig = 6;
int Echo = 7;

// モード切り替え関係
boolean mode_auto = false;        // auto: 自律制御
const int buff_size = 10;

// 対象ID
const int start_ID = 1;
const int end_ID = 127;
/**************************************/


// timeout check class
class TimeOut
{
  private:
    long timeout_time;
  
  public:
    // set timeout time width
    void set_timeout(long timeout)
    {
      this->timeout_time = millis() + timeout;
    }
    // timeout check, true: timeout
    boolean is_timeout()
    {
      if(millis() > this->timeout_time)
        return true;
      else
        return false;
    }
    // constructer
    TimeOut()
    {
      this->timeout_time = 0l;
    }
};


// millis()の代わりに利用する
// 同期を取りやすくするため
class kmTimer
{
private:
  long _time_origin;

public:
  void reset()
  {
    this->_time_origin = millis();
  }
  long km_millis()
  {
    return millis() - this->_time_origin;
  }
  // constructer
  kmTimer()
  {
    this->_time_origin = 0l;
  }
};


class Colormap
{
private:
  double StartH;
  double EndH;
  double S;
  double V;
  boolean Reverse;
  void GenerateColorMap(int start, int end, double saturation, double brightness)
  {
    if (start > end)
    {    
      this->Reverse = true;
      this->StartH = end % 360;
      this->EndH = start % 360;
    }
    else
    {
      this->Reverse = false;
      this->StartH = start % 360;
      this->EndH = end % 360;
    }

    if (saturation > 100) { 
      this->S = 100; 
    }
    else if (saturation < 0) { 
      this->S = 0; 
    }
    else { 
      this->S = saturation; 
    }

    if (brightness > 100) { 
      this->V = 100; 
    }
    else if (brightness < 0) { 
      this->V = 0; 
    }
    else { 
      this->V = brightness; 
    }
  }
  void ConvertHSVtoRGB(double _h, double _s, double _v, int* _r, int* _g, int* _b)
  {
    _s = _s / 100;
    _v = _v / 100;
    _h = (int)_h % 360;

    if (_s == 0.0)      //Gray
    {
      int rgb = (int)((double)(_v * 255));

      *_r = (int)(rgb);
      *_g = (int)(rgb);
      *_b = (int)(rgb);
    }
    else
    {
      int Hi = (int)((int)(_h / 60.0) % 6);
      double f = (_h / 60.0) - Hi;

      double p = _v * ( 1 - _s);
      double q = _v * (1 - f * _s);
      double t = _v * (1 - (1 - f) * _s);

      double r = 0.0;
      double g = 0.0;
      double b = 0.0;

      switch (Hi)
      {
      case 0:
        r = _v;
        g = t;
        b = p;
        break;

      case 1:
        r = q;
        g = _v;
        b = p;
        break;

      case 2:
        r = p;
        g = _v;
        b = t;
        break;

      case 3:
        r = p;
        g = q;
        b = _v;
        break;

      case 4:
        r = t;
        g = p;
        b = _v;
        break;

      case 5:
        r = _v;
        g = p;
        b = q;
        break;

      default:
        r = 0;
        g = 0;
        b = 0;
        break;
      }

      *_r = (int)(r * 255);
      *_g = (int)(g * 255);
      *_b = (int)(b * 255);
    }
    return;
  }
public:
  Colormap()
  {
    this->GenerateColorMap(240.0, 0, 100.0, 100.0);
  }
  void GetColor(double value, int* r, int* g, int* b)
  {
    if(value > 1.0) value = 1.0;
    if(value < 0.0) value = 0.0;
    if (this->Reverse == true)
    {
      value = 1.0 - value;
    }
    return this->ConvertHSVtoRGB(((this->EndH - this->StartH) * value) + this->StartH, this->S, this->V, r, g, b);
  }
};





/** global variabls part 2 **********************/
Colormap colmap;
kmTimer kmtimer;
// millis()を置換させる（同期をとるため）
#define millis() kmtimer.km_millis()




/** general functions ***************************/
void light(int r, int g, int b, int unit_id)
{
  r = r >> 1;
  g = g >> 1;
  b = b >> 1;
  
  // send header
  xbee_serial->write(header_v2);
  delay(5);

  // send RGB pattern
  int limit = 127;
  if(r > limit)
    r = limit;
  if(r < 0)
    r = 0;
  xbee_serial->write(r | 0x80);
  if(g > limit)
    g = limit;
  if(g < 0)
    g = 0;
  xbee_serial->write(g | 0x80);
  if(b > limit)
    b = limit;
  if(b < 0)
    b = 0;
  xbee_serial->write(b | 0x80);

  // send id
  xbee_serial->write(unit_id | 0x80);

  /*
  Serial.print(header_v2, HEX);
  Serial.print(",");  
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.print(b);
  Serial.print(",");
  Serial.print(unit_id);
  Serial.println("");
  */
    
  return;
}


// 色が行きつ戻りつゆっくり変化する
void light_pattern6()
{
  int r, g, b;
  
  // 色を決める
  float _color_phase = (float)((millis() + color_phase) % color_cycle_width) / (float)(color_cycle_width / 2l); // 0-2 色相環が循環しないので、ちょっと工夫
  if(_color_phase > 1.0f)
    _color_phase = -(_color_phase - 2.0f);
  Serial.print("color phase: ");
  Serial.println(_color_phase);
  colmap.GetColor(_color_phase, &r, &g, &b);
  
  // 明るさを決める
  float _brightness_phase = (float)((millis() + brightness_phase) % brightness_cycle) / (float)brightness_cycle; // 0-1
  Serial.print("brightness phase: ");
  Serial.println(_brightness_phase);
  float brightness = sin(_brightness_phase * 2.0f * 3.14f) * 50.0f + 100.0f; // max: 150, min: 50
  //brightness = 10.0f * log(brightness);
  Serial.print("brightness: ");
  Serial.println(brightness);
  r += 10; // 0だと、何を掛けても白っぽくなりえない
  g += 10;
  b += 10;
  r = (int)((long)r * (long)brightness / 100l);
  g = (int)((long)g * (long)brightness / 100l);
  b = (int)((long)b * (long)brightness / 100l);
  
  // ちらつかせてみる
  long rnd = random(0, 100);
  if(rnd > 97)
  {
    rnd = random(50, 100);
    r = (int)((long)r * rnd / 100l);
    g = (int)((long)g * rnd / 100l);
    b = (int)((long)b * rnd / 100l);
  }
  
  if(r > 255) r = 255;
  if(g > 255) g = 255;
  if(b > 255) b = 255;
  
  for(int _id = start_ID; _id <= end_ID; _id++)
    light(r, g, b, _id);
  return;
}


float check_distance_with_sonic()
{
  float distance = -1.0f;
  
  digitalWrite(Trig, LOW);
  delayMicroseconds(1);
  digitalWrite(Trig, HIGH);
  delayMicroseconds(1);
  digitalWrite(Trig, LOW);
  int duration = pulseIn(Echo, HIGH);
  if (duration > 0) {
    distance = duration / 2;
    distance = distance * 340 * 100 / 1000000; // ultrasonic speed is 340m/s = 34000cm/s = 0.034cm/us 
    /*
    Serial.print(duration);
    Serial.print(" us ");
    Serial.print(distance);
    Serial.println(" cm");
    */
  }
  return distance;
}




void setup() {
  // com
  Serial.begin(57600);
  #ifdef PLATFORM_IS_UNO
    _xbee_serial.begin(19200);
  #endif
  #ifdef PLATFORM_IS_MEGA
    Serial1.begin(19200);
  #endif
  
  // init rand
  randomSeed(analogRead(A0));
  
  // sonic
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
  
  // init variable
  color_cycle_width = color_cycle_width * random(70, 100) / 100l; // 周期に差をつける
  brightness_cycle = brightness_cycle * random(70, 100) / 100l;
  color_phase = random(0, color_cycle_width);
  brightness_phase = random(0, brightness_cycle);
  Serial.print("color_cycle_width: ");
  Serial.println(color_cycle_width);
  Serial.print("brightness_cycle: ");
  Serial.println(brightness_cycle);
  Serial.print("color_phase: ");
  Serial.println(color_phase);
  Serial.print("brightness_phase: ");
  Serial.println(brightness_phase);
  
  Serial.print("--start (wait 5 sec)--");
  delay(5000);
}

void loop() {
  // sonic check
  static const int ave_size = 2;
  static long distance_average = 200;
  
  distance_average = ((long)ave_size * distance_average + (long)check_distance_with_sonic()) / (long)(ave_size + 1);
  Serial.print("distance_average: ");
  Serial.println(distance_average);
  
  // 距離が短いと、強制的に遠隔操作モードに切り替える
  if(distance_average < 80l){
    for(int i = 0; i < buff_size * 3; i++)
      xbee_serial->write(header_v5);
    mode_auto = true;
  }else{
    for(int i = 0; i < buff_size * 3; i++)
      xbee_serial->write(header_v4);
    mode_auto = false;
  }
  
  // LED制御
  if(mode_auto == true) // 本当はクラス化した方がいいなぁ。
    light_pattern6();
}
