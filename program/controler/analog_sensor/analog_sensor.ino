void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int adc;
  
  adc = analogRead(A0);
  Serial.print(adc);
  Serial.print(",");
  adc = analogRead(A1);
  Serial.print(adc);
  Serial.print(",");
  adc = analogRead(A2);
  Serial.print(adc);
  Serial.print(",");
  adc = analogRead(A3);
  Serial.print(adc);
  Serial.print(",");
  adc = analogRead(A4);
  Serial.println(adc);
  
  delay(200);
}
