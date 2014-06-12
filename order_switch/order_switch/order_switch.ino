/* order_switch                             */
/* Author: Katsuhiro MORISHITA              */
/* created: 2014-06-13                      */
/* memo: IDの割り振りが不確定なので、変更の見込み。*/
/* software map:                            */
/*    Serial :     com with PC              */
/*    Serial1:     com with                 */
/*    Serial2:                              */
/*    Serial3:                              */
/*    Soft Serial:                          */
#include<AltSoftSerial.h>

const int header = 0x3f;

const long baud_pc = 115200l;
const long baud_xbee = 19200l;
const long baud_stage = 115200l;

AltSoftSerial serial_stage;

const int group1 = 0;
const int group2 = 12; // 19200 bpsだと1チャンネル当たり、12ユニット以下でないと、30 fpsは出ない
const int group3 = 24;
const int group4 = 36;

//--------------------------------------

// 指定されたシリアル出力リソースを利用して、データを出力する
void send_serial(Stream *s1, char buff[], int len)
{
  for(int i = 0; i < len; i++)
  {
    s1->write(buff[i]);
  }
  return;
}

// 
// if return 1, recall this function.
int get_len_and_data(char *buff, int *id, int *len)
{
  int index = 1;
  buff[0] = (char)header;
  *id = 0;
  *len = 0;
  
  while(1)
  {
    if(Serial.available())
    {
      int c = Serial.read();
      if(c == header)
      {
        buff[0] = 0x00;
        return 1;
      }
      int check = c & 0x008f;
      if(c == 0)               // 8 bit目は1のはず
      {
        buff[0] = 0x00;
        return 2;
      }
      buff[index] = (char)c;
      index += 1;
      if(index > 4)
      {
        *id = c;
        *len = 4;
        return 0;
      }
    }
  }
  return -1;
}


void setup()
{
  Serial.begin(baud_pc);
  Serial1.begin(baud_xbee);
  Serial2.begin(baud_xbee);
  Serial3.begin(baud_xbee);
  serial_stage.begin(baud_stage);
}

// PCからデータを受信しつつ、受信したIDに合わせて出力先を変更する
void loop()
{
  char buff[10];
  int len;
  int id;
  int condition = -1;
  
  if(Serial.available())
  {
    int c = Serial.read();
    if(c == header)
    {
      while(1)
      {
        condition = get_len_and_data(buff, &id, &len);
        if(condition != 1)
          break;
      }
    }
  }
  
  if(condition == 0)
  {
    if(id > 0)
    {
      if(id < group2)
        send_serial(&Serial1, buff, len);
      else if(id < group3)
        send_serial(&Serial2, buff, len);
      else if(id < group4)
        send_serial(&Serial3, buff, len);
      else
        send_serial(&serial_stage, buff, len);
    }
  }
}
