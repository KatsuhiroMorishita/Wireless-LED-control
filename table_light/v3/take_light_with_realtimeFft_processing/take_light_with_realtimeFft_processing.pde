/* name: realtimeFft                                           */
/* purpose: this program show FFT result of PC recording sound.*/
/* created: 2014-05-07                                         */
/* ref: http://macromarionette.com/computergraphics/cg9.html   */
import ddf.minim.analysis.*;
import ddf.minim.*;
Minim minim;
import processing.serial.*;

Serial myPort;
final byte pattern_code_header = 0x7f;
float[] coefficient = {0.8, 1.0, 0.3};

AudioInput in;
FFT fft;
float amp_global = 0.005;
float[] amplitude = {amp_global*100, amp_global*100, amp_global*100};
//int divide_coe = 10; // general music
//int divide_coe = 4;  // anime music
//int divide_coe = 6;  // piano, sacred song
int divide_coe = 2;  // bocaroid
//int divide_coe = 8;  // whistle
//int divide_coe = 20; // low man voice
//int divide_coe = 5;  // violin
//int divide_coe = 1;

//-------------------------------

// send light pattern
void light(int r, int g, int b, int[] on_off)
{
  // send header
  myPort.write(pattern_code_header);
  delay(1);
  
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
  myPort.write(r);
  if(g > limit)
    g = limit;
  if(g < 0)
    g = 0;
  myPort.write(g);
  if(b > limit)
    b = limit;
  if(b < 0)
    b = 0;
  myPort.write(b);
  
  // send on-off order
  int len = on_off.length;
  int loop_limit = len / 8 + 1;
  for(int i = 0; i < loop_limit; i++)
  {
    int val = 0x00;
    for(int k = 0; k < 8; k++)
    {
      val = (val << 1) & 0xfe;
      int j = i * 8 + k;
      if(j < len)
      {
        if(on_off[j] != 0)
          val = val | 0x01;
      }
    }
    myPort.write(val & 0x7f);
  }

  return;
}


void setup()
{
  // fft, etc.
  size(600, 300);
  minim = new Minim(this);
  in = minim.getLineIn(Minim.STEREO, 512);
  fft = new FFT(in.bufferSize(), in.sampleRate());
  
  // serial port setting
  String[] ports = Serial.list();
  println(ports);
  myPort = new Serial(this, ports[4], 9600);
  
  // test light pattern
  int hoge[] = {0, 1, 1, 0};
  light(255, 255, 255, hoge);
  delay(2000);
  for(int i = 0; i < hoge.length; i++){
    hoge[i] = 0;
  }
  light(0, 0, 0, hoge);
}


int calc_red(FFT _fft)
{
  float r = 0;
  int specSize = fft.specSize() / divide_coe;
  for (int i = 0; i < specSize; i++)
  {
    float x = (float)i / (float)specSize;
    float theta = x * 2.0 * 3.14;
    float v = cos(theta) + 1.0;
    v *= log(abs(fft.getBand(i)) + 1);
    if(x < 0.5)
      v = 0.0;
    r += v;
  }
  r *= amplitude[0] * divide_coe;
  print("red: ");
  println(r);
  return (int)r;
}

int calc_blue(FFT _fft)
{
  float b = 0;
  int specSize = fft.specSize() / divide_coe;
  for (int i = 0; i < specSize; i++)
  {
    float x = (float)i / (float)specSize;
    float theta = x * 2.0 * 3.14;
    float v = cos(theta) + 1.0;
    v *= log(abs(fft.getBand(i)) + 1);
    if(x > 0.5)
      v = 0.0;
    b += v;
  }
  b *= amplitude[2] * divide_coe;
  print("blue: ");
  println(b);
  return (int)b;
}


int calc_green(FFT _fft)
{
  float g = 0;
  int specSize = fft.specSize() / divide_coe;
  for (int i = 0; i < specSize; i++)
  {
    float x = (float)i / (float)specSize;
    float theta = x * 2.0 * 3.14;
    float v = -cos(theta) + 1.0;
    v *= log(abs(fft.getBand(i)) + 1);
    g += v;
  }
  g *= amplitude[1] * divide_coe;
  print("green: ");
  println(g);
  return (int)g;
}


void draw()
{
  // draw LED light order
  int r = calc_red(fft);
  int g = calc_green(fft);
  int b = calc_blue(fft);
  int[] test = {0, 1, 0, 1};
  light(r, g, b, test);
  background(r, g, b);
  
  // draw FFT result
  stroke(255);
  fft.forward(in.mix);
  int specSize = fft.specSize();
  for (int i = 0; i < specSize; i++)
  {
    float x = map(i, 0, specSize, 0, width);
    line(x, height, x, height - fft.getBand(i) * 8);
  }
}


void stop()
{
  minim.stop();
  super.stop();
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
  
  int[] test = {0, 1, 0, 1};
  light(r, g, b, test);
  
  // for check
  background(r, g, b);
  
}
