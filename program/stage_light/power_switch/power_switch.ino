/* Name: power_swicth                                                            */
/* Author: Katsuhiro Morishita                                                   */
/* purpose: 有線で接続されたステージのテープLEDの点灯を制御する・・・つもりだったコードの残骸 */
/*         点灯を制御することで、消費電力を抑えることが目的である。                       */
/* memo:   シリアルデータを転送しながら、LEDの電源を制御する。                           */
/*         シリアルデータの転送は、PCからのLED点灯パターン転送が目的である。               */
/*         D2, D3, D4ポートはLEDの電源を制御するFETに接続されている。                   */
/* Created: 2014-06-21                                                           */

const long baud = 9600l;

void setup()
{
  Serial.begin(baud);
  Serial1.begin(baud);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  
  
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
}

void loop()
{
  if(Serial.available())
  {
    int c = Serial.read();
    Serial1.write(c); 
  }
}
