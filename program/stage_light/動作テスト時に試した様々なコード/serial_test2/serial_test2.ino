// ただのエコーバック
// USBケーブルの延長がどこまで可能かどうかを確認するのに利用した。
void setup()
{
  Serial.begin(38400);
  Serial.println("echo back test start.");
}

void loop()
{

  if(Serial.available())
  {
    int c = Serial.read();
    Serial.write(c);
  }
}


