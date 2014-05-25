/* program name: take_light_arduino_v3                  */
/* author:  Katsuhiro MORISHITA                         */
/* :                                                    */
/* create:  2014-05-25                                  */
/* license:  MIT                                        */
/* format: header, R, G, B, lisht bit field             */
#include <SoftwareSerial.h>
#include <EEPROM.h>

// IO map
const int red_pin   = 9;
const int green_pin = 6;
const int blue_pin  = 5;
// header
const char header = 0x80;
// address
const int id_init = -1;
int my_id = id_init;
// pwm
char pwm_red = 0;
char pwm_green = 0;
char pwm_blue = 0;
// com
Stream *xbee;
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
  
  long wait_time_ms = 30000l;
  _port->print("please input ID. last ");
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
      if(c == header)
      {
        ans = 1;
        break;                    // 再帰は避けたい
      }
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
            analogWrite(red_pin  , _pwm_red);
            analogWrite(green_pin, _pwm_green);
            analogWrite(blue_pin , _pwm_blue);
            ans = 0;
            break;
          }
        }
      }
    }
  }
  return ans;
}


// setup
void setup()
{ 
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("-- start --");
  
  //SoftwareSeria ss;
  //ss.begin(9600);
  
  xbee = &Serial;
  
  // get ID
  int _id = recieve_id(Serial);
  if(_id >= 0)
  {
    my_id = _id;
    Serial.println("-- write id to EEPROM --");
    EEPROM.write(0, my_id);  // msb first ?
    delay(100);
  }
  else
  {
    Serial.println("-- read id from EEPROM --");
    byte v1 = EEPROM.read(0);
    byte v2 = EEPROM.read(1);
    my_id = (int)v1 << 8 + (int)v2; // msb first ?
    //my_id = (int)v2 << 8 + (int)v1; // lsb first ?
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
}

// loop
void loop()
{
  if(xbee->available())
  {
    int c = xbee->read();
    if((char)c == header)
    {
      while(1)
      {
        char ans = receive_light_pattern(xbee);
        if(ans == 0)
          break;
      }
    }
  }
}



