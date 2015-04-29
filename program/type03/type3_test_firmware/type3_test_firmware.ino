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
const char header = 0x7f;
const char header_v2 = 0x3f;
const char header_v3 = 0x4f; // 内部時計のリセット（同期用）
const char header_v4 = 0x5f; // オートモードへ切り替え

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

// 色が段々変わる。色の変化はノコギリ波
void light_pattern1()
{
  int r, g, b;
  // 発光テスト
  for(int k = 0; k < 100; k++)
  {
    colmap.GetColor((double)k / 100, &r, &g, &b);
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println("");

    //delay(100);
    for(int i = 0; i < NUMPIXELS; i++)
    {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }
  return;
}


// 色んな色が流れるように光る
void light_pattern2()
{
  int r, g, b;

  float f = 0.2;
  float X = 0.3;
  float dx = 0.02; // [mm]
  float power = 0.05; // max: 1.0

  for(int i = 0; i < 100; i++)
  {
    for(int i = 0; i < NUMPIXELS; i++)
    {
      float t = (float)millis() / 1000.0f;
      float x = i * dx;
      float theta = 2.0 * 3.14 * (f * t + x / X);
      float y = (sin(theta) + 1.0f) / 2.0f;
      colmap.GetColor(y, &r, &g, &b);
      r = (int)((float)r * power); // power save
      g = (int)((float)g * power);
      b = (int)((float)b * power);
      Serial.print(r);
      Serial.print(",");
      Serial.print(g);
      Serial.print(",");
      Serial.print(b);
      Serial.println("");

      //delay(100);

      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }
  return;
}

// 点灯が流れるように発光する
void light_pattern3()
{
  int _max = 20;
  int r = random(0, _max), g = random(0, _max), b = random(0, _max);


  for(int i = 0; i < NUMPIXELS; i++)
  {
    int index = i;
    int index_pre = i - 1;
    if(i == 0) index_pre = NUMPIXELS - 1;
    pixels.setPixelColor(index, pixels.Color(r, g, b)); // Moderately bright green color.
    pixels.setPixelColor(index_pre, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(random(1, 100));
  }

  r = random(0, _max), g = random(0, _max), b = random(0, _max);
  for(int i = NUMPIXELS; i > 0; i--)
  {
    int index = i;
    int index_pre = i + 1;
    if(i == NUMPIXELS - 1) index_pre = 0;
    pixels.setPixelColor(index, pixels.Color(r, g, b)); // Moderately bright green color.
    pixels.setPixelColor(index_pre, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(random(1, 100));
  }
  return;
}



// ホテル発光
void light_pattern4()
{
  int _max = 20;
  int r = random(0, _max), g = random(0, _max), b = random(0, _max);
  int index = random(0, NUMPIXELS);
  long t;
  long term = 1500;


  for(int k = 0; k < random(1, 10); k++)
  {
    t = millis();
    long tw = 0l;
    while(tw < term)
    {
      float scale = sin(3.14 * (float)tw / (float)term);
      int _r = (int)((float)r * scale);
      int _g = (int)((float)g * scale);
      int _b = (int)((float)b * scale);
      pixels.setPixelColor(index, pixels.Color(_r, _g, _b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      tw = millis() - t;
    }
    delay(200);
  }

  int _delay = 2000;
  delay(random(_delay, _delay * 4));
  return;
}



// 梵字点灯
void light_pattern5()
{
  const int _max = 255;
  static int r, g, b;
  static long t;
  static long term = 4000l; // 1回の点灯時間
  static long tw = 0l;
  static int width = 5;     // 奇数のこと
  static float anplitude[] = {
    0.1, 0.7, 1.0, 0.7, 0.1  }; // len == width, sin()で良いなら、初期化は以下の中に埋め込む
  static int index;
  static TimeOut to;
  static boolean mode_turn_on = true;
  static boolean mode_turn_on_backup = false;


  if(mode_turn_on == true){
    if(mode_turn_on_backup == false){ // renew
      r = random(0, _max); // 色相はここで決まる
      g = random(0, _max);
      b = random(0, _max);
      index = random(width - 1, NUMPIXELS - (int)(width / 2));
      mode_turn_on_backup = mode_turn_on;
      t = millis();
      to.set_timeout(term);
    }
    // light
    float scale = sin(3.14 * (float)tw / (float)term);
    int _r = (int)((float)r * scale); // 時間とともに、輝度を変える
    int _g = (int)((float)g * scale);
    int _b = (int)((float)b * scale);
    for(int k = 0; k < width; k++){
      int dk = k - (int)(width / 2);
      pixels.setPixelColor(index + dk, pixels.Color(_r * anplitude[k], _g * anplitude[k], _b * anplitude[k])); // 微妙に残光があるかも？
    } 
    pixels.show();
    tw = millis() - t;
    // check time-out
    if(to.is_timeout() == true)
      mode_turn_on = false;
  }
  else{
    if(mode_turn_on_backup == true){ // renew to obj
      int _delay = 1000;
      to.set_timeout(random(_delay, _delay * 2));
      mode_turn_on_backup = mode_turn_on;
      light(0, 0, 0);                // たまに通信が上手く行かなくて光り続けるLEDがあるやつの対策。
    }
    // check time-out
    if(to.is_timeout() == true)
      mode_turn_on = true;
  }
  return;
}



void light_pattern6()
{
  int r, g, b;
  // 発光テスト
  for(int k = 0; k < 100; k++)
  {
    colmap.GetColor((double)k / 100, &r, &g, &b);
    r = (int)((float)r * power_scale);
    g = (int)((float)g * power_scale);
    b = (int)((float)b * power_scale);
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println("");

    //delay(100);
    for(int i = 0; i < NUMPIXELS; i++)
    {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }
  for(int k = 100; k > 0; k--)
  {
    colmap.GetColor((double)k / 100, &r, &g, &b);
    r = (int)((float)r * power_scale);
    g = (int)((float)g * power_scale);
    b = (int)((float)b * power_scale);
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println("");

    //delay(100);
    for(int i = 0; i < NUMPIXELS; i++)
    {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }
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
      if(c == header || c == header_v2)
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
        if ((index - 4) == my_index)
        {
          int bit_location = my_id % 8;
          int masked_c = (c >> (7 - bit_location)) & 0x01;
          if (masked_c)
          {
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
      if(c == header || c == header_v2)
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
  static TimeOut to;
  boolean data_receive = false;

  if(to.is_timeout() == true)  // もし制御信号が検出されないことが続いたらオートに戻すための仕掛け
    mode_auto = true;

  // LED制御
  if(mode_auto == true) // 本当はクラス化した方がいいなぁ。
    light_pattern6();

  // 受信データ処理
  if(xbee->available())
  {
    int c = xbee->read();
    Serial.println(c);
    // 制御パターン1
    if((char)c == header)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern(xbee);
        if(ans == 0)
          data_receive = true;
        if(ans != 1)
          break;
      }
    }
    // 制御パターン2
    if((char)c == header_v2)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern_v2(xbee);
        if(ans == 0)
          data_receive = true;
        if(ans != 1)
          break;
      }
    }
    // しばらく、制御内容を保持する
    if(data_receive == true){
      mode_auto = false;
      to.set_timeout(30000l);  // もし制御信号が検出されないことが続いたらオートに戻すための仕掛け
    }
    // 時刻同期のし掛け
    if((char)c == header_v3){
      kmtimer.reset();
    }
    // 強制でオートモードに切り替えるコードを受信した場合
    if((char)c == header_v4){
      mode_auto == true;
    }
  }
}



