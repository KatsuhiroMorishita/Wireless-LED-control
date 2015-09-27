/*********************************************
purpose: 花畑町のTAMAIREイベントのために、投入されたボールを検出するセンサーを検出する。
 全部で12チャンネル使う。
 チャンネル毎に感度が異なる。
bug: 106行目のコメントを見て欲しい。バックアップしたカウント値を出力しないと行けなかった・・・。
author: Katsuhiro Morishita
created: 2015-09-22
*********************************************/
#include <AltSoftSerial.h>

AltSoftSerial altSerial;


const int SENSOR_LENGTH = 12;       // センサー数


const int _SIZE = 3; 
int buff[_SIZE][SENSOR_LENGTH];
int wp[SENSOR_LENGTH];
const int TH = 350;
int th[SENSOR_LENGTH];              // センサー毎に閾値を帰れたらいいだろうなと思って配列にした
boolean detect[SENSOR_LENGTH];

const int header = 0x31;
const int terminal = 0xff;

const int count_max = 0x89;
const int count_min = 0x80;
int count[SENSOR_LENGTH];

const int msg_count_max = 0xfe;
const int msg_count_min = 0x8a;
int msg_count = msg_count_min;

const int interval = 200;
long _time = 0l;
long _old_time = 0l;

void setup() {
  for(int i = 0; i < SENSOR_LENGTH; i++)   // 変数の初期化
  {
    count[i] = count_min;
    wp[i] = 0;
    detect[i] = false;
    th[i] = TH;
  }
  
  // put your setup code here, to run once:
  Serial.begin(19200);
  altSerial.begin(38400);
  altSerial.println("Hello World");
  delay(1000);
  
  Serial.print("th: ");
  for(int i = 0; i < SENSOR_LENGTH; i++)
    Serial.println(th[i]);
  delay(3000);
}

void loop() 
{
  for(int i = 0; i < SENSOR_LENGTH; i++)
  {
    int a = analogRead(A0 + i);
    //Serial.print("adc: ");
    //Serial.println(a);
    buff[wp[i]][i] = a;
    wp[i] += 1;
    wp[i] %= _SIZE;
    
    long sum = 0l;
    for(int k = 0; k < _SIZE; k++)
      sum += (long)buff[k][i];
    long ave = sum / (long)_SIZE;
    //Serial.print("ave: ");
    //Serial.println(ave);
    
    if(ave > th[i]){
      //Serial.println("high");
      if(detect[i] == false)
        count[i] += 1;
      if(count[i] > count_max)
        count[i] = count_max;
      detect[i] = true;
    }else{
      //Serial.println("low");
      detect[i] = false;
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
      
      // 本番用
      /**/
      Serial.write(header);
      Serial.write(msg_count);
      for(int k = 0; k < SENSOR_LENGTH; k++){
        int j = 0; // 回路設定ミスって、その処理に複雑なことに・・・
        j = (int)(k / _SIZE) * _SIZE + ((_SIZE - 1) - (k % _SIZE));
        Serial.write(count[j]);                  // よく考えると、リアルタイムのカウント値を出力したらだめじゃん
      }
      Serial.write(terminal);
      /**/
      
      // test msg
      /*
      Serial.print(",");
      for(int k = 0; k < SENSOR_LENGTH; k++){
        Serial.print(count[k] - count_min);
        Serial.print(",");
      }
      Serial.println("");
      */
      
  
      msg_count += 1;
      if(msg_count > msg_count_max)
        msg_count = msg_count_min;
      for(int i = 0; i < SENSOR_LENGTH; i++)
        count[i] = count_min;
      _time = 0l;
    }
  }

  delay(2);    // ≒センシング周期
}
