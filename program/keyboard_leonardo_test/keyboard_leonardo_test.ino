// Arduino leonardoでUSBキーボードを作る例


void send_key(char c)
{
  // press keykeyboardkeyboard
  for(int i = 0; i < 2; i++)
    Serial.write(0);
  for(int i = 0; i < 6; i++)
    Serial.write(c);
  
  // release  
  for(int i = 0; i < 8; i++)
    Serial.write(0);
  return;
}

void setup()
{
  Serial.begin(9600);
  delay(5000);
  send_key('a');
  send_key('b');
  send_key('c');
  send_key('d');
}

void loop()
{
  delay(1000);
  send_key('a');
}
