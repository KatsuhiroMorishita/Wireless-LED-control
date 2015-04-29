/* program name: keyboard_leonardo                      */
/* purpose: USBキーボードとして機能させ、プロジェクションマッピ */
/*          ングとの同期に利用する。                        */
/* memo: 透過モードにあるXBeeからのシリアルデータをキー入力する。*/
/* author:  Katsuhiro MORISHITA                         */
/* create:  2015-01-04                                  */
/* license:  MIT                                        */
#include <SoftwareSerial.h>

SoftwareSerial mySerial(8, 9);

void setup()
{
  Keyboard.begin();
  mySerial.begin(4800);
  
  delay(5000); // 待たないと、スケッチの書き換えに支障を生じうる
}

void loop()
{
  if(mySerial.available())
  {
    int c = mySerial.read();
    Keyboard.write(c);
  }
}
