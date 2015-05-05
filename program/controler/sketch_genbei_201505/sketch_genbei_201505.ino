/* program name: sketch_genbei_kai                                    */
/* author:  Izumi Shinobu, Katsuhiro MORISHITA                        */
/* purpose: 三井ホールでゲンベイさんの展示を光らせつつ、天井照明を制御する。     */
/* create:  2015-05-04                                                */
/* license:  MIT                                                      */
/* memo: Arduino IDE 1.6以上でないと、コンパイルでエラーがでる。             */
#include <AltSoftSerial.h>

#define PLATFORM_IS_UNO
//#define PLATFORM_IS_MEGA

#ifdef PLATFORM_IS_UNO
AltSoftSerial _xbee_serial(8,9); // port is fixed on 8 & 9 at UNO.
Stream* xbee_serial = &_xbee_serial;
#endif

#ifdef PLATFORM_IS_MEGA
Stream* xbee_serial = &Serial1;
#endif

const char pattern_code_header = 0x3f;

class TimeredProcess{
  private:
    long count = 0;
    
    int times1;
    int times2;
    
    int id = 0;
  
  public:
  TimeredProcess(int _id)
  {
    id = _id;
  }
  
  void process(){
    
    if(count == 0){
      times1 = random(10,200);
      times2 = times1 + random(20, 200);
    }
    
    count++;
    
    if(count <= times1){
      int v = random(0,2);
      
        
      if(v == 0){
        int f = random(30, 127);
        light(f, f, f, id);
      }
      if(v != 0){
        light(0, 0, 0, id);
        //int f = random(0, 100);
        //light(f, f, f, id);
      }
        
      delay(10);
    }else if(count <= times2){
      int f = random(0, 100);
      light(0, 0, 0, id);
      delay(10);
    }else{
      count = 0;
    } 
  } 
};

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

  Serial.print(pattern_code_header);
  Serial.print(",");  
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.print(b);
  Serial.print(",");
  Serial.print(unit_id);
  Serial.println("");
    
  return;
}


void setup()
{
  // 通信のセットアップ
  Serial.begin(115200);
  
  #ifdef PLATFORM_IS_UNO
    _xbee_serial.begin(19200);
  #endif
  
  #ifdef PLATFORM_IS_MEGA
    Serial1.begin(19200);
  #endif

}


TimeredProcess tp1(39); // ゲンベイコーナー
TimeredProcess tp2(41);
TimeredProcess tp3(42);
TimeredProcess tp4(43);
TimeredProcess tp5(44);
TimeredProcess tp6(45);
TimeredProcess tp7(46);
TimeredProcess tp8(47);

void loop()
{
  tp1.process();
  tp2.process();
  tp3.process();
  tp4.process();
  tp5.process();
  tp6.process();
  tp7.process();
  tp8.process();
  
  // 天井のライト
  static long turn_on_time = 0l;
  static long turn_off_time = 0l;
  char io_switch = digitalRead(2);
  if(io_switch == HIGH && turn_on_time + 1000l < millis()){ // チャタリングによる点滅は避けたい（電球の寿命という面で）
    light(1,1,0,99);
    turn_on_time = millis();
  }
  if(io_switch == LOW && turn_off_time + 1000l < millis()){
    light(0,0,0,99);
    turn_off_time = millis();
  }
}

