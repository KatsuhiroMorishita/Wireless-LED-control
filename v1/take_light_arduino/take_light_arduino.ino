/* program name:                 */
/* author:  Katsuhiro Morishita  */
/* :                             */
/* create:  2014-04-29           */
/* license:  MIT                 */
#include <MsTimer2.h>


// IO map
const int red_pin   = 9;
const int green_pin = 5;
const int blue_pin  = 6;
// global
const int list_size = 200;    // if you need more sram, you should use sram I2C device.
int duty_list[3][list_size];
const int red = 0;
const int green = 1;
const int blue = 2;

const int escape_code = 0x7e;  // like XBee escape
const int escape_char = 0x02;
const int recieve_light_pattern_code = 'a';
const int start_light_code = 'b';
const int stop_light_code = 'c';
int code_list[] = {
  recieve_light_pattern_code, 
  start_light_code, 
  stop_light_code,
  escape_code,
  escape_char};  // specific function char

unsigned long backup_time_millis = 0l;

boolean light_enable = false;

int max_index = 0;             // store litht pattern size
int now_light_index = 0;       // be incremented in timer interrupt
/**************************************/

// software timer interrupt
// change led light pattern
void light_led()
{
  if(light_enable == true)
  {
    analogWrite(red_pin  , duty_list[red][now_light_index]);
    analogWrite(green_pin, duty_list[green][now_light_index]);
    analogWrite(blue_pin , duty_list[blue][now_light_index]);
    
    now_light_index += 1;
    now_light_index %= max_index;
    Serial.println("change.");
  }
  else
  {
    analogWrite(red_pin  , 0);
    analogWrite(green_pin, 0);
    analogWrite(blue_pin , 0);
  }
  return;
}

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

// send message
void send_msg(char *msg, int buff_size)
{
  for(int i = 0; i < buff_size; i++)
  {
    char c = msg[i];
    boolean escape_need = false;
    for(int k = 0; k < sizeof(code_list); k++)
    {
      if(c == code_list[k])
      {
        Serial.write(escape_code);
        escape_need = true;
        break;
      }
    }
    if(escape_need == true)
      Serial.write(escape_char ^ c);
    else
      Serial.write(c);
  }
}

// recieve light pattern
void receive_light_pattern()
{
  int color = 0;
  int index = 0; 
  boolean timeout = false;
  boolean escape = false;

  (void)set_time_for_timeout();
  while(timeout == false)
  {
    int c = Serial.read();
    if(c != -1)
    {
      (void)set_time_for_timeout();  // extend timeout
      if(c == escape_code)
        escape = true;
      else
      {
        if(escape == true)
        {
          c ^= escape_char;
          escape = false;
        }
        duty_list[color][index] = c;
        color += 1;
        if(color >= 3)                  // color is 3, red, green, blue.
        {
          color = 0;
          index += 1;
          max_index = index;
        }
        if(index > list_size)
          break;
      }
    }
    timeout = check_timeout(2000ul);
  }
  if(timeout == true)
    max_index += -1;
  if(max_index < 0)
    max_index = 0;
  return;
}


// setup
void setup()
{ 
  Serial.begin(9600);
  Serial.println("wake up!");
  
  MsTimer2::set(100, light_led); // software timer interrupt every 100ms
  MsTimer2::start();
}

// loop
void loop()
{
  int c = Serial.read();
  if(c == recieve_light_pattern_code)
  {
    Serial.println("pateern recieve start.");
    (void)receive_light_pattern();
    Serial.println("pateern was received.");
  }
  else if(c == start_light_code)
  {
    now_light_index = 0;
    light_enable = true;
    Serial.println("turn on");
  }
  else if(c == stop_light_code)
  {
    light_enable = false;
    Serial.println("turn off");
  }
}

