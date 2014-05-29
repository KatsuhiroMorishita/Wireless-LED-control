/* program name: take_light_processing_v2               */
/* author:  Katsuhiro Morishita                         */
/* :                                                    */
/* create:  2014-04-29                                  */
/* license:  MIT                                        */
/* format: code, address, red, green, blue, time width  */
/* address format: gggg iiii, upper 4 bit is group      */
/*                            under 4 bit is id         */
import processing.serial.*;

Serial myPort;
final int recieve_light_pattern_code = 'a';


void light(int r, int g, int b, int time_width)
{
  // send light pattern
  myPort.write((byte)recieve_light_pattern_code);
  delay(1);
  byte group = 0;
  byte id = 0;
  byte address = (byte)((group << 4) & id);
  myPort.write(address);
  myPort.write(r);
  myPort.write(g);
  myPort.write(b);
  myPort.write(time_width);

  // for check
  background(r, g, b);
  return;
}


void setup() 
{
  // window size
  size(800, 400);
  //frameRate(60);
  
  // serial port setting
  String[] ports = Serial.list();
  println(ports);
  myPort = new Serial(this, ports[3], 9600); // can i search available serial port?
  
  light(255, 255, 255, 200);
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
  int v = (key - 0x30) * 10;
  println(v);
  light(v * 2, v * 2, v * 2, 5);
}
