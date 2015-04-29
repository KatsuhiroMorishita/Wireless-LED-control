
// 指定されたシリアル出力リソースを利用して、データを出力する
// ポインタの使い方が正しいかどうかを確認するのに使用した。
void send_serial(Stream *s1, char buff[], int len)
{
  for(int i = 0; i < len; i++)
  {
    s1->write(buff[i]);
  }
  return;
}

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
}



void loop()
{
  char buff[] = "hogehoge fugafuga.";
  int len = sizeof(buff);
  
  send_serial(&Serial1, buff, len);
  delay(100);
}
