/* program name: take_light_processing_header1_v5               */
/* author:  Katsuhiro Morishita                         */
/* :                                                    */
/* create:  2014-06-06                                  */
/* license:  MIT                                        */
/* format: code, red, green, blue, on-off control       */
import processing.serial.*;

// target
final int target_unit_list[] = {0, 1, 1, 0}; // 1: ON, 0: OFF

// com
Serial myPort;
final int baudrate = 19200;
final byte pattern_code_header = 0x7f;
float[] coefficient = {0.8, 1.0, 0.3};
//-------------------------------

// send light pattern
void light(int r, int g, int b, int[] _target_unit_list)
{
  // send header
  myPort.write(pattern_code_header);
  delay(5);
  
  // tuning
  r = (int)((float)r * coefficient[0]);
  g = (int)((float)g * coefficient[1]);
  b = (int)((float)b * coefficient[2]);
  
  // send RGB pattern
  int limit = 127;
  if(r > limit)
    r = limit;
  if(r < 0)
    r = 0;
  myPort.write(r | 0x80);
  if(g > limit)
    g = limit;
  if(g < 0)
    g = 0;
  myPort.write(g | 0x80);
  if(b > limit)
    b = limit;
  if(b < 0)
    b = 0;
  myPort.write(b | 0x80);
  
  // send on-off order
  int len = _target_unit_list.length;
  int loop_limit = len / 8 + 1;
  for(int i = 0; i < loop_limit; i++)
  {
    int val = 0x00;
    for(int k = 0; k < 8; k++)
    {
      val = val << 1;
      int j = i * 8 + k;
      if(j < len)
      {
        if(_target_unit_list[j] != 0)
          val = val | 0x01;
      }
    }
    myPort.write(val | 0x80);
  }

  return;
}


void setup() 
{
  // window size
  size(400, 400);
  
  // serial port setting
  String[] ports = Serial.list();
  println(ports);
  myPort = new Serial(this, ports[4], baudrate);
  
  // test light pattern
  light(255, 255, 255, target_unit_list);
  delay(2000);
  light(0, 0, 0, target_unit_list);
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

// keyboard event
// send light pattern to LED control module.
void keyPressed()
{
  int i = (key - 0x30);
  print("key: ");
  println(i);
  
  float pi = 3.14;
  float theta = i / 9.0 * 2.0 * pi;
  int range = 127 / 2;
  float offset = 120.0 / 180.0 * pi;
  int r = (int)(sin(theta + offset * 0) * range + range);
  int g = (int)(sin(theta + offset * 1) * range + range);
  int b = (int)(sin(theta + offset * 2) * range + range);
  if(i == 0){ // if you press '0', turn off LED.
    r = 0;
    g = 0;
    b = 0;
  }
  print("r, g, b: ");
  print(r);
  print(", ");
  print(g);
  print(", ");
  print(b);
  println("");
  
  light(r, g, b, target_unit_list);
  
  // for check
  background(r, g, b);
  
}
