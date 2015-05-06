/* program name: type04_firmware                        */
/* author:  Katsuhiro MORISHITA                         */
/* purpose: 笑平さんの体にぺたぺた貼り付けて光らせる。         */
/* create:  2015-04-29                                  */
/* license:  MIT                                        */
/* format: header, R, G, B, lisht bit field             */
/* platform: UNO                                        */
#include <AltSoftSerial.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            6

// How many NeoPixels are attached to the Arduino?
const int NUMPIXELS = 30;//16 * 3 + 12;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// header
const char header_color = 0x7f;

// pwm
char pwm_red = 0;
char pwm_green = 0;
char pwm_blue = 0;
// com
long usbserial_baudrate = 115200;
long xbee_baudrate = 19200;
Stream *xbee;
AltSoftSerial _xbee_serial(8, 9); // ポートはArduionによって異なる


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


// recieve light pattern
// return: char, 1: 再度呼び出しが必要
char receive_light_pattern(Stream *port)
{
  char ans = 2;
  TimeOut to;
  int index = 0;
  
  int rgbCount = 0;   // これを使って受信データをrgbに分類する
  int count = 0;      // シリアルテープLEDの個々のLEDの番号
  int r;
  int g;
  int b;
  
  //Serial.println("-- p1 --");
  to.set_timeout(100);
  while(to.is_timeout() == false)
  {
    
    if(port->available())
    {
      int c = port->read();
      //Serial.print("-- c --: ");
      //Serial.print(c);
      //Serial.println("");
      
      index += 1;
      if(c == header_color)
      {
        Serial.println(index);
        Serial.println("-- header duplicated --");
        ans = 1;
        break;                    // 再帰は避けたい
      }
      if((c & 0x80) == 0)         // プロトコルの仕様上、ありえないコードを受信
      {
        Serial.println("-- invalid data --");
        ans = 2;
        break;                    // エラーを通知して、関数を抜ける
      }
      c = c & 0x7f;               // 最上位ビットにマスク
      to.set_timeout(100);
      if (index <= NUMPIXELS*3 && c >= 0 && c < 128) { // cがpwm設定値で、かつ2倍してもchar最大値を超えない場合
        //c *= 2;
//        if(c < 20)
 //         c = 0;
        
        if(rgbCount == 0) {
          r = c;
          r = r << 1;
          rgbCount = 1;
        }else if(rgbCount == 1){
          g = c;
          g = g << 1;
          rgbCount = 2;
        }else{
          b = c;
          b = b << 1;
          rgbCount = 0;
          
          /*
          Serial.print(count);Serial.print(':');
          Serial.print(r);Serial.print(',');Serial.print(g);Serial.print(',');Serial.print(b);
          Serial.println(' ');
          */
          
          pixels.setPixelColor(count, pixels.Color(r, g, b)); // Moderately bright green color.
          count += 1;
        }
      }
    } 
  }
  pixels.show();
  return ans;
}

// setup
void setup()
{ 
  pixels.begin(); // This initializes the NeoPixel library.
  
  Serial.begin(usbserial_baudrate);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(1000);
  Serial.println("-- setup start --");
  
  _xbee_serial.begin(xbee_baudrate);
  xbee = &_xbee_serial;// &Serial;
  
  // light test pattern
  Serial.println("-- test turn on --");
  for(int i=0;i<127; i++){
    test_seq(i, i, i);
    delay(1000/127);
  }
  test_seq(0, 0, 0);
  Serial.println("-- setup end --");
  Serial.println("-- stand-by --");
}

// loop
void loop()
{
  if(xbee->available())
  {
    int c = xbee->read();
    //Serial.print("xbee:");
    //Serial.println(c);
    if((char)c == header_color)
    {
      receive_light_pattern(xbee);
    }
  }
}

void test_seq(int r, int g, int b)
{
  
  for(int i = 0; i < NUMPIXELS; i++)
  {
    int index = i;
    pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
  }
   pixels.show(); // This sends the updated pixel color to the hardware.
  
  return;
}

