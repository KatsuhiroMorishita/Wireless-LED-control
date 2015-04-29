/* program name: take_light_processing_header2_v5       */
/* author:  Katsuhiro Morishita                         */
/* plat form: Processing 2.2.1                          */
/* create:  2014-06-06                                  */
/* license:  MIT                                        */
/* format: code, red, green, blue, on-off control       */
import processing.serial.*;

// target
final int unit_id = 1;

// com
Serial myPort;
final int baudrate = 19200;
final byte pattern_code_header = 0x3f;
float[] coefficient = {0.8, 1.0, 0.3};
//-------------------------------

// send light pattern
void light(int r, int g, int b, int unit_id)
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
  
  // send id
  myPort.write(unit_id | 0x80);
  print("ID: ");
  println(unit_id);

  return;
}


void setup() 
{
  // window size
  size(400, 400);
  
  // serial port setting
  String[] ports = Serial.list();
  println(ports);
  String port = "";      // only available that usbserial is just one.
  for (int i = 0 ; i < ports.length; i++)
  {
    port = ports[i];
    if(match(port, ".*usbserial.*") != null)
      break;
  }
  myPort = new Serial(this, port, baudrate);
  
  // test light pattern
  light(255, 255, 255, unit_id);
  delay(2000);
  light(0, 0, 0, unit_id);
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
  
  light(r, g, b, unit_id);
  
  // for check
  background(r, g, b);
  
}
