/* program name: take_light_arduino_v5s                  */
/* author:  Katsuhiro MORISHITA                          */
/* purpose: ステージに並べられたテープLEDを制御するためのコード  */
/* memo:    take_light_arduino_v5を流用した。              */
 /* create:  2014-06-22                                  */
/* license:  MIT                                         */
/* format: header, R, G, B, lisht bit field              */
/* memo: 1) 本プログラム内のLEDの色指定は間違っています。        */
/*       2) ATemega328P 内蔵8MHzで動作することを前提にしています*/
#include <AltSoftSerial.h>
#include <EEPROM.h>

// IO map
const int red_pin   = 6;
const int green_pin = 3;
const int blue_pin  = 5;
// header
const char header = 0x7f;
const char header_v2 = 0x3f;
// address
const int id_init = -1;
int my_id = id_init;
// pwm
char pwm_red = 0;
char pwm_green = 0;
char pwm_blue = 0;
// com
long usbserial_baudrate = 38400;
long xbee_baudrate = 38400;
Stream *xbee;
const int xbee_pin = 8;
AltSoftSerial _xbee_serial(8, 9); // 11 pin is no connect!
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
void light(int r, int g, int b)
{
  analogWrite(red_pin  , r);
  analogWrite(green_pin, g);
  analogWrite(blue_pin , b);
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

  xbee->println("-- p1 --");
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

  xbee->println("-- p2 --");
  to.set_timeout(20);
  while(to.is_timeout() == false)
  {
    if(port->available())
    {
      int c = port->read();
      index += 1;
      if(c == header || c == header_v2)
      {
        ans = 1;
        break;                    // 再帰は避けたい
      }
      if((c & 0x80) == 0)         // プロトコルの仕様上、ありえないコードを受信
      {
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
  return ans;
}


// setup
void setup()
{ 
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);

  Serial.begin(usbserial_baudrate);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(1000);
  Serial.println("-- setup start --");

  _xbee_serial.begin(xbee_baudrate);
  xbee = &_xbee_serial;// &Serial;

  
  // get ID
  int _id = recieve_id(xbee);
  if(_id >= 0 && _id != id_init)
  {
    my_id = _id;
    xbee->println("-- write id to EEPROM --");
    byte v_h = (byte)((my_id & 0xff00) >> 8);
    EEPROM.write(0, v_h);
    xbee->println(v_h);
    byte v_l = (byte)(my_id & 0x00ff);
    EEPROM.write(1, v_l);
    xbee->println(v_l);
    int parity = (int)v_h ^ (int)v_l;
    EEPROM.write(2, (byte)parity);
    xbee->println(parity);
    delay(100);
  }
  else
  {
    xbee->println("-- read id from EEPROM --");
    byte v_h = EEPROM.read(0);
    byte v_l = EEPROM.read(1);
    byte v_p = EEPROM.read(2);
    xbee->println(v_h);
    xbee->println(v_l);
    xbee->println(v_p);
    int parity = (int)v_h ^ (int)v_l;
    if(parity == v_p)
      my_id = (int)v_h * 256 + (int)v_l;
    else
      xbee->println("-- parity is not match --");
  }
  xbee->println("-- my ID --");
  xbee->println(my_id);

  // ID check
  if(my_id == id_init)
  {
    xbee->println("-- ID error --");
    xbee->println("-- program end --");
    for(;;);
  }
  xbee->println("-- ID check OK --");

  // light test pattern
  xbee->println("-- test turn on --");
  light(100, 100, 100);
  delay(1000);
  light(0, 0, 0);

  xbee->println("-- setup end --");
  xbee->println("-- stand-by --");
  
}

// loop
void loop()
{
  if(Serial.available())
  {
    int c = Serial.read();
    //xbee->println("c");
    if((char)c == header)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern(&Serial);
        if(ans != 1)
          break;
      }
    }
    if((char)c == header_v2)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern_v2(&Serial);
        if(ans != 1)
          break;
      }
    }
  }
}


