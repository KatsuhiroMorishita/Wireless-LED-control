/* program name: take_light_arduino_v3                  */
/* author:  Katsuhiro MORISHITA                         */
/* :                                                    */
/* create:  2014-05-25                                  */
/* license:  MIT                                        */
/* format: header, R, G, B, lisht bit field             */
#include <SoftwareSerial.h>
#include <EEPROM.h>

// IO map
const int red_pin   = 6;
const int green_pin = 9;
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
Stream *xbee;
const int xbee_pin = 12;
SoftwareSerial _xbee_serial(xbee_pin, 11); // 11 pin is no connect!
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
  
  Serial.begin(19200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(1000);
  Serial.println("-- setup start --");
  
  _xbee_serial.begin(19200);
  
  xbee = &_xbee_serial;// &Serial;
  
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
  light(100, 100, 100);
  delay(1000);
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
    if((char)c == header)
    {
      //light(100, 100, 100);
      while(1)
      {
        char ans = receive_light_pattern(xbee);
        if(ans != 1)
          break;
      }
    }
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

