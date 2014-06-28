// Serialから受信したシリアル情報をソフトウェアシリアルへ転送する。
// 転送しながら、接続された3色LEDの発光テストを実施する。

#include<AltSoftSerial.h>

const long baud = 19200l;
AltSoftSerial usbserial;
long time = 0l;
int phi = 0;

void setup()
{
  Serial.begin(baud);
  usbserial.begin(baud);
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  
  digitalWrite(3, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  
  delay(1000);
}

void loop()
{
  if(Serial.available())
  {
    int c = Serial.read();
    usbserial.write(c); 
  }
  
  // 順繰り点灯させる
  if(millis() - time > 1000l)
  {
    if(phi == 0)
    {
      digitalWrite(3, HIGH);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
    }
    if(phi == 1)
    {
      digitalWrite(3, LOW);
      digitalWrite(5, HIGH);
      digitalWrite(6, LOW);
    }
    if(phi == 2)
    {
      digitalWrite(3, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, HIGH);
    }
    
    phi = (phi + 1) % 3;
    time = millis();
  }
}
