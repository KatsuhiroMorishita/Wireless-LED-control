/* program name:                                        */
/* author:  Katsuhiro Morishita                         */
/* :                                                    */
/* create:  2014-04-29                                  */
/* license:  MIT                                        */
/* format: code, address, red, green, blue, time width  */
/* address format: gggg iiii, upper 4 bit is group      */
/*                            under 4 bit is id         */

// IO map
const int red_pin   = 9;
const int green_pin = 6;
const int blue_pin  = 5;
// address
const char group_for_all = 0;
const char group_me      = 1;
const char id_me         = 0;
// code
const int code_size = 4;
int operation_code[code_size];
const int red = 0;
const int green = 1;
const int blue = 2;
const int time_width = 3;

const int recieve_light_pattern_code = 'a';

unsigned long backup_time_millis = 0l;

boolean light_enable = false;
/**************************************/

// set now time, for check timeout of communication
void set_time_for_timeout()
{
  backup_time_millis = millis();
  return;
}

// check timeout for communication
// time unit is [ms].
boolean check_timeout(unsigned long time_span_millis)
{
  if(millis() > backup_time_millis+ time_span_millis)
    return true;
  else
    return false;
}

// recieve light pattern
char receive_light_pattern()
{
  char ans = 0;
  int index = -1; 
  boolean timeout = false;
  int _operation_code[code_size];

  (void)set_time_for_timeout();
  while(timeout == false)
  {
    int c = Serial.read();
    if(c != -1)
    {
      (void)set_time_for_timeout();   // extend timeout
      if(index == -1)                  // id check
      {
        char group_id = (c >> 4) & 0x0f;
        char id = c & 0x0f;
        boolean flag = false;

        if(group_id == group_for_all)
          flag = true;
        else if(group_id == group_me && id == id_me)
          flag = true;

        if(flag == false)
          break;
        else
          index += 1;
      }
      else
      {
        _operation_code[index] = c;
        index += 1;
        if(index >= code_size)
        {
          for(int i = 0; i < code_size; i++)       // copy code
            operation_code[i] = _operation_code[i];
          ans = 1;
          break;
        }
      }
    }
    timeout = check_timeout(25ul);
  }
  return ans;
}

// led light
void light_led()
{
  boolean timeout = false;
  unsigned long _time_width = (unsigned long)operation_code[time_width] * 10;

  analogWrite(red_pin  , operation_code[red]);
  analogWrite(green_pin, operation_code[green]);
  analogWrite(blue_pin , operation_code[blue]);

  (void)set_time_for_timeout();
  while(timeout == false)
  {
    timeout = check_timeout(_time_width);
  }
  analogWrite(red_pin  , 0);
  analogWrite(green_pin, 0);
  analogWrite(blue_pin , 0);
  return;
}


// setup
void setup()
{ 
  Serial.begin(9600);
  Serial.println("wake up!");
}

// loop
void loop()
{
  int c = Serial.read();
  if(c == recieve_light_pattern_code)
  {
    Serial.println("pateern recieve start.");
    char ans = receive_light_pattern();
    Serial.println("pateern was received.");
    if(ans == 1)
      (void)light_led();
  }
}



