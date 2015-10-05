/*********************************************
purpose: 花畑町のTAMAIREイベントのために、投入されたボールを検出する反応の閾値を判断するために生のAD返還値を取得する。
author: Katsuhiro Morishita
created: 2015-09-22
*********************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
}

void loop() {
  // put your main code here, to run repeatedly:
  int adcs;
  
  for(int i = A0; i <= A11; i++)
  {
    int adc = analogRead(i);
    //adcs[i] = adc;
    Serial.print(adc);
    Serial.print(",");
  }
  Serial.println("");
  delay(100);
}
