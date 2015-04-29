/* program name: type3_test                             */
/* author:  Katsuhiro MORISHITA                         */
/* :                                                    */
/* create:  2015-01-03                                  */
/* license:  MIT                                        */
/* format 1: header_v2, R, G, B, lisht bit field        */
#include <AltSoftSerial.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define PLATFORM_IS_UNO
//#define PLATFORM_IS_MEGA

/** global variabls　part1 **********************/
// header
const char header_v2 = 0x3f;

// serial LED setup
const int NUMPIXELS = 16 * 3 + 12;
const int IO_PIN = 6;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, IO_PIN, NEO_GRB + NEO_KHZ800);
const float briteness_max_for_all_on = 0.7f;

// own address
const int id_init = -1;
int my_id = id_init;

// com
long usbserial_baudrate = 57600;
long xbee_baudrate = 38400;
Stream *xbee;

// other
boolean mode_auto = true;

#ifdef PLATFORM_IS_UNO
AltSoftSerial _xbee_serial(8,9); // port is fixed on 8 & 9 at UNO.
#endif

#ifdef PLATFORM_IS_UNO
float power_scale = 0.3;
#elif
float power_scale = 1.0;
#endif

/** class *************************************/
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




/** general functions ***************************/

// IDを受信する
// IDは文字列で受信する。
// 例："1234"
int recieve_id(Stream &port)
{
  TimeOut to;
  int id = id_init;
  Stream *_port = &port;

  long wait_time_ms = 5000l;
  _port->print("please input ID. within ");
  _port->print(wait_time_ms);
  _port->println(" ms.");

  to.set_timeout(wait_time_ms);
  while(to.is_timeout() == false)
  {
    if(_port->available())
    {
      int c = _port->read();
      if (c < '0' && c > '9')
      {
        _port->print("-- Error: non number char was received --");
        break;
      }
      if (id == id_init)
        id = 0;
      to.set_timeout(50);            // 一度受信が始まったらタイムアウトを短くして、処理時間を短縮する
      id = id * 10 + (c - '0');
      if(id >= 3200)                 // 3200以上の数値を10倍するとint型の最大値を超える可能性がある
        break;
    }
  }
  if(id != id_init)
    _port->print("-- ID was received --");
  return id;
}





// LED control
// 全てのLEDを単色に光らせる
void light(int r, int g, int b)
{
  r = (int)(briteness_max_for_all_on * (float)r);
  g = (int)(briteness_max_for_all_on * (float)g);
  b = (int)(briteness_max_for_all_on * (float)b);
  for(int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    //pixels.show();
  }
  pixels.show();
  return;
}



// recieve light pattern v2
// return: char, 1: 再度呼び出しが必要
char receive_light_pattern_v2(Stream *port)
{
  char ans = 2;
  int my_index = my_id;
  TimeOut to;
  int index = 0;
  int _pwm_red = 0;
  int _pwm_green = 0;
  int _pwm_blue = 0;

  Serial.println("-- p2 --");
  to.set_timeout(20);
  while(to.is_timeout() == false)
  {
    if(port->available())
    {
      int c = port->read();
      //Serial.println(c);
      index += 1;
      if(c == header_v2)
      {
        //Serial.println("fuga");
        ans = 1;
        break;                    // 再帰は避けたい
      }
      if((c & 0x80) == 0)         // プロトコルの仕様上、ありえないコードを受信
      {
        //Serial.println("hoge");
        ans = 2;
        break;                    // エラーを通知して、関数を抜ける
      }
      c = c & 0x7f;               // 最上位ビットにマスク
      to.set_timeout(20);
      if (index < 4 && c > 0 && c < 128) // cがpwm設定値で、かつ2倍してもchar最大値を超えない場合
        c *= 2;
      if(index == 1)
      {
        _pwm_red = c;
      }
      else if(index == 2)
      {
        _pwm_green = c;
      }
      else if(index == 3)
      {
        _pwm_blue = c;
      }
      else // 点灯するかどうかの判断
      {
        if (c == my_index)
        {
          light(_pwm_red, _pwm_green, _pwm_blue);
          ans = 0;
          break;
        }
      }
    }
  }
  //if(to.is_timeout() == true)
  //  Serial.println("time out.");
  return ans;
}








// setup
void setup()
{ 
  // serial LED setup
  pixels.begin(); // This initializes the NeoPixel library.

  // serial com setting
  Serial.begin(usbserial_baudrate);
  delay(1000);
  Serial.println("-- setup start --");

#ifdef PLATFORM_IS_UNO
  _xbee_serial.begin(xbee_baudrate);
  xbee = &_xbee_serial;
#else
  Serial1.begin(xbee_baudrate);
  xbee = &Serial1;
#endif

  // get ID
  int _id = recieve_id(Serial);
  if(_id >= 0 && _id != id_init)
  {
    my_id = _id;
    Serial.println("-- write id to EEPROM --");
    byte v_h = (byte)((my_id & 0xff00) >> 8);
    EEPROM.write(0, v_h);
    Serial.println(v_h);
    byte v_l = (byte)(my_id & 0x00ff);
    EEPROM.write(1, v_l);
    Serial.println(v_l);
    int parity = (int)v_h ^ (int)v_l;
    EEPROM.write(2, (byte)parity);
    Serial.println(parity);
    delay(100);
  }
  else
  {
    Serial.println("-- read id from EEPROM --");
    byte v_h = EEPROM.read(0);
    byte v_l = EEPROM.read(1);
    byte v_p = EEPROM.read(2);
    Serial.println(v_h);
    Serial.println(v_l);
    Serial.println(v_p);
    int parity = (int)v_h ^ (int)v_l;
    if(parity == v_p)
      my_id = (int)v_h * 256 + (int)v_l;
    else
      Serial.println("-- parity is not match --");
  }
  Serial.println("-- my ID --");
  Serial.println(my_id);

  // ID check
  if(my_id == id_init)
  {
    Serial.println("-- ID error --");
    Serial.println("-- program end --");
    for(;;);
  }
  Serial.println("-- ID check OK --");

  // light test pattern
  Serial.println("-- test turn on --");
  int level = 255;
  light(level, level, level);
  delay(1000);
  //while(1);
  light(0, 0, 0);

  Serial.println("-- setup end --");
  Serial.println("-- stand-by --");
}







// loop
void loop()
{
  if(xbee->available())
  {
    int c = xbee->read();
    Serial.println(c);
    if((char)c == header_v2)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern_v2(xbee);
        if(ans != 1)
          break;
      }
    }
  }
}

