/*********************************************
purpose: 花畑町のTAMAIREイベントのために、投入されたボールを検出するセンサーを検出する。
 全部で1チャンネル使う。アルゴリズムの確認用
author: Katsuhiro Morishita
created: 2015-09-10
*********************************************/
#include <AltSoftSerial.h>

AltSoftSerial altSerial;


const int _size = 3; 
int buff[_size];
int wp = 0;
int th = (2 << (10 - 1)) * 1 / 5;  // 閾値が1V
boolean detect = false;

const int header = 0x31;
const int terminal = 0xff;

const int count_max = 0x89;
const int count_min = 0x80;
int count = count_min;

const int msg_count_max = 0xfe;
const int msg_count_min = 0x8a;
int msg_count = msg_count_min;

const int interval = 100;
long _time = 0l;
long _old_time = 0l;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
  altSerial.begin(38400);
  altSerial.println("Hello World");
  delay(1000);
  
  Serial.print("th: ");
  Serial.println(th);
  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  int a = analogRead(A0);
  //Serial.print("adc: ");
  //Serial.println(a);
  buff[wp++] = a;
  wp %= _size;
  long sum = 0l;
  for(int i = 0; i < _size; i++)
    sum += (long)buff[i];
  long ave = sum / (long)_size;
  //Serial.print("ave: ");
  //Serial.println(ave);
  
  if(ave > th){
    //Serial.println("high");
    if(detect == false)
      count += 1;
    if(count > count_max)
      count = count_max;
    detect = true;
  }else{
    //Serial.println("low");
    detect = false;
  }
  //Serial.print("count: ");
  //Serial.println(count);
  
  _time += millis() - _old_time;
  _old_time = millis();
  if(_time > interval){
    //Serial.print("msg: ");
    //Serial.print(msg_count);
    //Serial.print(",");
    //Serial.println(count);
    
    /**/
    Serial.print(header);
    Serial.print(msg_count);
    Serial.print(count);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.print(count_min);
    Serial.println(terminal);
    /**/
    
    /*
    Serial.write(header);
    Serial.write(msg_count);
    Serial.write(count);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(count_min);
    Serial.write(terminal);
    */
    
    /*
    Stream *debug_serial;
    debug_serial = &altSerial;
    debug_serial->print(header);
    debug_serial->print(",");
    debug_serial->print(msg_count);
    debug_serial->print(",");
    debug_serial->print(count);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->print(0);
    debug_serial->print(",");
    debug_serial->println(terminal);

    Stream *xbee;
    xbee = &Serial;
    xbee->write((char)header);
    xbee->write((char)msg_count);
    xbee->write((char)count);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)0);
    xbee->write((char)terminal);
    */
    
    msg_count += 1;
    if(msg_count > msg_count_max)
      msg_count = msg_count_min;
    count = count_min;
    _time = 0l;
  }
  
  
  
  delay(10);
}


