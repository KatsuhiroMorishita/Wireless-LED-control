/* program name: take_light_processing_header2_v5       */
/* author:  Katsuhiro Morishita                         */
/* memo: */
/* plat form: Processing 2.2.1                          */
/* create:  2014-06-06                                  */
/* license:  MIT                                        */
/* format: code, red, green, blue, on-off control       */
import processing.serial.*;

// target
final int module_id_start = 100;
final int module_id_end = 130;

// com
Serial myPort;
final int baudrate = 19200;
final byte pattern_code_header = 0x3f;
float[] coefficient = {0.8, 1.0, 0.8};
float power = 0.1;
//-------------------------------

// send light pattern
void light(int r, int g, int b, int unit_id)
{
  // send header
  myPort.write(pattern_code_header);
  delay(5);
  
  // tuning
  r = (int)((float)r * coefficient[0] * power);
  g = (int)((float)g * coefficient[1] * power);
  b = (int)((float)b * coefficient[2] * power);
  
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
  println(str(unit_id));
  
  // show ID for Front
  text("hoge", 0, 0, 300, 300);

  return;
}


void pattern01()
{
  float omega = 0.05;
  float pi = 3.141592;
  float t = (float)millis() / 1000.0;
  float theta = 2.0 * pi * omega * t;
  int range = 100 / 2;
  float offset = 120.0 / 180.0 * pi;
  int r = (int)(sin(theta + offset * 0) * range + range);
  int g = (int)(sin(theta + offset * 1) * range + range);
  int b = (int)(sin(theta + offset * 2) * range + range);
  background(r, g, b);
  for(int _id = module_id_start; _id <= module_id_end; _id++)
  {
    //light(r, g, b, _id);
    /**/
    for(int i = 0; i <= 1; i++)
    {
      int k = _id + i * 5;
      if(k > module_id_end) k = module_id_start + k - module_id_end;
      if(k < module_id_start) k = module_id_end + k - module_id_start;
      light(r, g, b, k);
    }
    /**/
  }
}

void pattern02()
{
  float omega = 0.05;
  float pi = 3.141592;
  float t = (float)millis() / 1000.0;
  float theta = 2.0 * pi * omega * t;
  int range = 100 / 2;
  float offset = 120.0 / 180.0 * pi;
  int r = (int)(sin(theta + offset * 0) * range + range);
  int g = (int)(sin(theta + offset * 1) * range + range);
  int b = (int)(sin(theta + offset * 2) * range + range);
  background(r, g, b);
  for(int _id = module_id_start; _id <= module_id_end; _id++){
    light(r, g, b, _id);
    delay(100);
    light(0, 0, 0, _id);
  }
}


void setup() 
{
  // window size
  size(400, 400);
  frameRate(30);
  textSize(40);
  fill(0);
  
  // serial port setting
  String[] ports = Serial.list();
  println(ports);
  String port = "COM18";      // for Windows
  /**/
  for (int i = 0 ; i < ports.length; i++)
  {
    port = ports[i];
    if(match(port, ".*usbserial.*") != null)
      break;
  }
  /**/
  myPort = new Serial(this, port, baudrate);
  
  // test light pattern
  for(int _id = module_id_start; _id <= module_id_end; _id++)
    light(255, 255, 255, _id);
  delay(2000);
  for(int _id = module_id_start; _id <= module_id_end; _id++)
    light(0, 0, 0, _id);
}

void draw()
{
  pattern01();
}

void serialEvent(Serial myPort) 
{
  String msg = myPort.readStringUntil('\n');
  //text(msg, 50, 50);
  //String msg = myPort.readString();
  if(msg != null)
    println(trim(msg));
}
