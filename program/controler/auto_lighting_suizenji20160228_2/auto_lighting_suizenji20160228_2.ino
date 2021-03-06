/* program name: auto_lighting_suizenji20160228_2       */
/* author:  Katsuhiro MORISHITA                         */
/* purpose: 水前寺公園の恋明かりのために、LEDの発光色を変える   */
/* Fioでつかおうと思ったが、使えなかったのでUNOになった。       */
/* create:  2016-02-27                                  */
/* license:  MIT                                        */
#include <AltSoftSerial.h> // UNO

// target ID
const int id_start = 1;
const int id_end = 20;

// serial
long usbserial_baudrate = 38400;
long xbee_baudrate = 38400;
AltSoftSerial _xbee_serial(8, 9);    // UNO
Stream* xbee_serial;

// header
const char header = 0x7f;    // 多数のモジュールを一つのコマンドで操作する（ノード毎のグラデーションがない状況に適している）
const char header_v2 = 0x3f; // 個々のモジュールに発光色を4byteで伝える
const char header_v3 = 0x4f; // 内部時計のリセット（同期用）
const char header_v4 = 0x5f; // オートモードへ切り替え
const char header_v5 = 0x6f; // 遠隔操作モードへ切り替え
const char pattern_code_header = header_v2;

// apmlitude
uint8_t amp_flags[id_end];  // 点滅させるノードのフラグを格納する


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


// 点灯させる
void light(int r, int g, int b, int unit_id)
{
  // send header
  xbee_serial->write(pattern_code_header);
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
  return;
}


// 発光色を変更する
// val:位相, _max:位相の最大値
void change_color_group(int val, int _max)
{
  int r, g, b;
  static Colormap colmap;
  
  for(int id = id_start; id <= id_end; id++)
  {
    if(id % 8 == 0) continue;
    colmap.GetColor((double)val / _max, &r, &g, &b);
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println("");

    //delay(1);
    light(r, g, b, id);
  }
  return;
}

// 発光強度を変更する
// x: 位相（0-1.0）, change_flag: 振幅を変えるなら1
void change_amp_group(float x, int r, int g, int b, uint8_t change_flag)
{
  int _r, _g, _b;
    
  float _amp = cos(x * 2.0 * 3.14) / 2.0 + 0.5;  // 振幅を決める
  float factor = 1.0f;                           // 発光強度の上限
  r = (int)((float)r * factor);
  g = (int)((float)g * factor);
  b = (int)((float)b * factor);
  
  for(int id = id_start; id <= id_end; id++)
  {
    if(id % 8 == 0) continue;                    // 8の倍数は飛ばす
    if(amp_flags[id] == 1 && change_flag == 1){
      _r = (int)((float)r * _amp);
      _g = (int)((float)g * _amp);
      _b = (int)((float)b * _amp);
    }else{
      _r = r;
      _g = g;
      _b = b;
    }
    
    Serial.print(_r);
    Serial.print(",");
    Serial.print(_g);
    Serial.print(",");
    Serial.print(_b);
    Serial.println("");

    //delay(1);
    light(_r, _g, _b, id);
  }
  return;
}

// 点滅するノードを変更する
void reset_amp()
{
  for(int i = 0; i < id_end; i++)
  {
    uint8_t _rand = (uint8_t)random(0, 10);
    if(_rand > 7)       // この数字で、色が変わる対象の数が決まる
      amp_flags[i] = 1;
    else
      amp_flags[i] = 0;
  }

  return;
}



void setup()
{
  // 通信のセットアップ
  Serial.begin(usbserial_baudrate);
  
  _xbee_serial.begin(xbee_baudrate); // UNO
  xbee_serial = &_xbee_serial; // UNO
  //xbee_serial = &Serial;       // Fio
  
  // val init
  reset_amp();
}


void loop()
{
  // 発光
  const int _max = 100;
  //for(int i = 0; i < _max; i++) change_color_group(i, _max);
  //for(int i = _max; i > 0; i--) change_color_group(i, _max);
  
  
  const int r = 255, g = 55, b = 147;
  for(int i = 0; i < 5; i++)
    for(int i = 0; i < _max; i++) change_amp_group((float)i / (float)_max, r, g, b, 1);
  for(int i = 0; i < 3; i++)
    for(int i = 0; i < _max; i++) change_amp_group((float)i / (float)_max, r, g, b, 0);
    
  reset_amp();
  
}







