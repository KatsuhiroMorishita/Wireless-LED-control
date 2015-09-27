/* program name: take_light_arduino_v5                  */
/* author:  Katsuhiro MORISHITA                         */
/* :                                                    */
/* create:  2014-06-06                                  */
/* license:  MIT                                        */
/* format: header, R, G, B, lisht bit field             */
#include <AltSoftSerial.h>
#include <EEPROM.h>

// IO map
const int red_pin   = 6;
const int green_pin = 5;
const int blue_pin  = 3;

// header
const char header = 0x7f;    // 多数のモジュールを一つのコマンドで操作する（ノード毎のグラデーションがない状況に適している）
const char header_v2 = 0x3f; // 個々のモジュールに発光色を4byteで伝える
const char header_v3 = 0x4f; // 内部時計のリセット（同期用）
const char header_v4 = 0x5f; // オートモードへ切り替え
const char header_v5 = 0x6f; // 遠隔操作モードへ切り替え

// address
const int id_init = -1;
int my_id = id_init;

// pwm
char pwm_red = 0;
char pwm_green = 0;
char pwm_blue = 0;

// LED setup
const float briteness_scale = 0.5f;
long color_cycle_width = 120000l;        // 半周期にかける時間 ms
long brightness_cycle = 30000l;
long color_phase = 0l;
long brightness_phase = 0l;

// com
long usbserial_baudrate = 57600;
long xbee_baudrate = 19200;
Stream *xbee;
AltSoftSerial _xbee_serial(8, 9); // 使えるピンはArduinoによって異なる。

// モード切り替え関係
boolean mode_auto = true;        // auto: 自律制御
const int buff_size = 10;
char buff[buff_size];
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

// IDを受信する
// IDは文字列で受信する。
// 例："1234"
int recieve_id(Stream *_port)
{
  TimeOut to;
  int id = id_init;
  //Stream *_port = &port;

  long wait_time_ms = 5000l;
  _port->print("please input ID. within ");
  _port->print(wait_time_ms);
  _port->println(" ms.");
  
  to.set_timeout(wait_time_ms);
  _port->flush();
  while(to.is_timeout() == false)
  {
    if(_port->available())
    {
      int c = _port->read();
      //_port->print("r: ");
      //_port->println(c);
      if (c == 0x0a || c == 0x0d)
        break;
      if (c < '0' || c > '9')
      {
        _port->print("-- Error: non number char was received --");
        break;
      }
      if (id == id_init)
        id = 0;
      to.set_timeout(50);            // 一度受信が始まったらタイムアウトを短くして、処理時間を短縮する
      id = id * 10 + (c - '0');
      if(id >= 3200)                 // 3200以上の数値を10倍するとint型の最大値を超える可能性がある.負数は無視。
        break;
    }
  }
  if(id != id_init)
    _port->print("-- ID was received --");
  //_port->print("r ID: ");
  //_port->println(id);
  return id;
}


// LED control
void light(int r, int g, int b)
{
  r = (int)(briteness_scale * (float)r);
  g = (int)(briteness_scale * (float)g);
  b = (int)(briteness_scale * (float)b);
  
  analogWrite(red_pin  , r);
  analogWrite(green_pin, g);
  analogWrite(blue_pin , b);
  
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.println(b);
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
  
  light(r, g, b);
  return;
}





// recieve light pattern
// return: char, 1: 再度呼び出しが必要
char receive_light_pattern(Stream *port)
{
  char ans = 2;
  int my_index = my_id / 8;
  TimeOut to;
  int index = 0;
  int _pwm_red = 0;
  int _pwm_green = 0;
  int _pwm_blue = 0;
  
  Serial.println("-- p1 --");
  to.set_timeout(20);
  while(to.is_timeout() == false)
  {
    if(port->available())
    {
      int c = port->read();
      //Serial.print("-- c --: ");
      //Serial.print(c);
      //Serial.println("");
      index += 1;
      if(c == header)
      {
        //Serial.println("-- d --");
        ans = 1;
        break;                    // 再帰は避けたい
      }
      if((c & 0x80) == 0)         // プロトコルの仕様上、ありえないコードを受信
      {
        //Serial.println("-- e --");
        ans = 2;
        break;                    // エラーを通知して、関数を抜ける
      }
      c = c & 0x7f;               // 最上位ビットにマスク
      to.set_timeout(20);
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
        if ((index - 4) == my_index)
        {
          int bit_location = my_id % 8;
          int masked_c = (c >> (7 - bit_location)) & 0x01;
          if (masked_c)
          {
            _pwm_red = _pwm_red << 1;
            _pwm_green = _pwm_green << 1;
            _pwm_blue = _pwm_blue << 1;
            light(_pwm_red, _pwm_green, _pwm_blue);
            ans = 0;
            break;
          }
        }
      }
    }
  }
  return ans;
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
          _pwm_red = _pwm_red << 1;
          _pwm_green = _pwm_green << 1;
          _pwm_blue = _pwm_blue << 1;
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
  // init IO
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  
  // init com
  Serial.begin(usbserial_baudrate);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(1000);
  Serial.println("-- setup start --");
  
  _xbee_serial.begin(xbee_baudrate);
  xbee = &_xbee_serial;// &Serial;
  
  // init rand
  randomSeed(analogRead(A0));
  
  // get ID
  int _id = recieve_id(&Serial);
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
  
  // init variable
  // モード切り替え用の変数
  for(int i = 0; i < buff_size; i++)
    buff[i] = 0x00;
  // other
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
  
  
  // light test pattern
  Serial.println("-- test turn on --");
  light(100, 100, 100);
  delay(1000);
  light(0, 0, 0);
  
  Serial.println("-- setup end --");
  Serial.println("-- stand-by --");
}



// loop
void loop()
{
  static int wp = 0;
  
  // LED制御
  if(mode_auto == true) // 本当はクラス化した方がいいなぁ。
    light_pattern6();
    
  // 受信データ処理
  if(xbee->available())
  {
    int c = xbee->read();
    buff[wp] = (char)c;
    wp = (wp + 1) % buff_size;
    Serial.println(c);
    
    // 制御パターン2
    if((char)c == header_v2 && mode_auto == false)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern_v2(xbee);
        if(ans != 1)
          break;
      }
    }
    
    // モード切り替え処理
    boolean header_compleat;
    
    // 時刻同期
    header_compleat = true;
    for(int i = 0; i < buff_size; i++)
      if(buff[i] != header_v3)
        header_compleat = false;
    if(header_compleat == true)
      kmtimer.reset();
    
    // 強制でオートモードに切り替えるコードを受信した場合
    header_compleat = true;
    for(int i = 0; i < buff_size; i++)
      if(buff[i] != header_v4)
        header_compleat = false;
    if(header_compleat == true)
      mode_auto = true;
    
    // 強制で遠隔操作モードに切り替えるコードを受信した場合
    header_compleat = true;
    for(int i = 0; i < buff_size; i++)
      if(buff[i] != header_v5)
        header_compleat = false;
    if(header_compleat == true)
      mode_auto = false;
  }
}

