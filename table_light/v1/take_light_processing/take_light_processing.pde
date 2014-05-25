import processing.serial.*;

Serial myPort;
final int escape_code = 0x7e;  // like XBee escape
final int escape_char = 0x02;
final int recieve_light_pattern_code = 'a';
final int start_light_code = 'b';
final int stop_light_code = 'c';
final int code_list[] = {
  recieve_light_pattern_code, 
  start_light_code, 
  stop_light_code,
  escape_code,
  escape_char};  // specific function char

int[] create_test_pattern(float freq, float time_step, float offset, int size)
{
  int bias = 256 / 2;
  int amplitude = 126;
  int[] ans = new int[size];
  
  //text("fuga", 0, 10);
  offset = offset / 180 * PI;
  for(int i = 0; i < size; i++)
  {
    int value = (int)(sin(2 * PI * freq * i * time_step + offset) * amplitude + bias);
    ans[i] = value;
    //point(i*10, value);
    //text(value, 0, 10 * (i + 1) + 10);
    //text("hoge", 0, 10 * (i + 2));
  }
  return ans;
}


int[] create_test_pattern2(float freq, float time_step, float offset, int size)
{
  int bias = 256 / 2;
  int amplitude = 126;
  int[] ans = new int[size];
  
  //text("fuga", 0, 10);
  offset = offset / 180 * PI;
  for(int i = 0; i < size; i++)
  {
    float s = sin(2 * PI * freq * i * time_step + offset);
    int ss = -1;
    if(s > 0)
      ss = 1;
    int value = (int)(ss * amplitude + bias);
    ans[i] = value;
    //point(i*10, value);
    //text(value, 0, 10 * (i + 1) + 10);
    //text("hoge", 0, 10 * (i + 2));
  }
  return ans;
}


// send message
void send_msg(int[] msg, int buff_size)
{
  for(int i = 0; i < buff_size; i++)
  {
    int c = msg[i];
    boolean escape_need = false;
    for(int k = 0; k < code_list.length; k++)
    {
      if(c == code_list[k])
      {
        myPort.write((byte)escape_code);
        escape_need = true;
        break;
      }
    }
    if(escape_need == true)
    {
      myPort.write((byte)(escape_char ^ c));
      text("escape fugafuga.", 50, 50);
    }
    else
    {
      myPort.write((byte)c);
    }
  }
}


void setup() 
{
  size(800, 400);
  //frameRate(60);
  
  String[] ports = Serial.list();
  println(ports);
  myPort = new Serial(this, ports[0], 9600); // can i search available serial port?
  
  // create light pattern
  int size = 10;
  int[] red   = create_test_pattern2(1.1, 0.1,   0, size);
  int[] green = new int[size];//create_test_pattern2(1.1, 0.1, 120, size);
  int[] blue  = new int[size];//create_test_pattern2(1.1, 0.1, 240, size);
  
  // send light pattern
  myPort.write((byte)recieve_light_pattern_code);
  delay(200);
  for(int i = 0; i < size; i++)
  {
    int[] msg = {red[i], green[i], blue[i]};
    delay(1);
    send_msg(msg, 3);
  }
  delay(3000);
  myPort.write((byte)start_light_code);
  
  // plot for check
  strokeWeight(5) ; 
  int hoge[][] = {red, green, blue};
  for(int k = 0; k < 3; k++)
  {
    for(int i = 0; i < size; i++)
    {
      point(i * 10, hoge[k][i]);
      text(str(hoge[k][i]), k * 50, 10 * (i + 1) + 10);
    }
  }
  /*
  int y = 0;
  for(int i = 0; i < size; i++)
  {
    text(hoge[i], 0, y);
    y += 12;
  }
  */
}

void draw()
{
  
}

void serialEvent(Serial myPort) 
{
  String msg = myPort.readStringUntil('\n');
  //text(msg, 50, 50);
  //String msg = myPort.readString();
  if(msg != null)
    println(trim(msg));
}

