/* program name: serial_LED_test                        */
/* author:  Katsuhiro MORISHITA                         */
/* purpose: シリアルLEDのテスト用スケッチ                   */
/* create:  2014-12-xx                                  */
/* license:  MIT                                        */
#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            6

// How many NeoPixels are attached to the Arduino?
const int NUMPIXELS = 16 * 3 + 12;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 500; // delay for half a second


// カラーマップ生成用のクラス
// C#で組んでいたものをC++に移植した。
class Colormap
{
private:
  double StartH;
  double EndH;
  double S;
  double V;
  boolean Reverse;
  void GenerateColorMap(int start, int end, double saturation, double brightness)
  {
    if (start > end)
    {    
      this->Reverse = true;
      this->StartH = end % 360;
      this->EndH = start % 360;
    }
    else
    {
      this->Reverse = false;
      this->StartH = start % 360;
      this->EndH = end % 360;
    }

    if (saturation > 100) { 
      this->S = 100; 
    }
    else if (saturation < 0) { 
      this->S = 0; 
    }
    else { 
      this->S = saturation; 
    }

    if (brightness > 100) { 
      this->V = 100; 
    }
    else if (brightness < 0) { 
      this->V = 0; 
    }
    else { 
      this->V = brightness; 
    }
  }
  void ConvertHSVtoRGB(double _h, double _s, double _v, int* _r, int* _g, int* _b)
  {
    _s = _s / 100;
    _v = _v / 100;
    _h = (int)_h % 360;

    if (_s == 0.0)      //Gray
    {
      int rgb = (int)((double)(_v * 255));

      *_r = (int)(rgb);
      *_g = (int)(rgb);
      *_b = (int)(rgb);
    }
    else
    {
      int Hi = (int)((int)(_h / 60.0) % 6);
      double f = (_h / 60.0) - Hi;

      double p = _v * ( 1 - _s);
      double q = _v * (1 - f * _s);
      double t = _v * (1 - (1 - f) * _s);

      double r = 0.0;
      double g = 0.0;
      double b = 0.0;

      switch (Hi)
      {
      case 0:
        r = _v;
        g = t;
        b = p;
        break;

      case 1:
        r = q;
        g = _v;
        b = p;
        break;

      case 2:
        r = p;
        g = _v;
        b = t;
        break;

      case 3:
        r = p;
        g = q;
        b = _v;
        break;

      case 4:
        r = t;
        g = p;
        b = _v;
        break;

      case 5:
        r = _v;
        g = p;
        b = q;
        break;

      default:
        r = 0;
        g = 0;
        b = 0;
        break;
      }

      *_r = (int)(r * 255);
      *_g = (int)(g * 255);
      *_b = (int)(b * 255);
    }
    return;
  }
public:
  Colormap()
  {
    this->GenerateColorMap(240.0, 0, 100.0, 100.0);
  }
  void GetColor(double value, int* r, int* g, int* b)
  {
    if(value > 1.0) value = 1.0;
    if(value < 0.0) value = 0.0;
    if (this->Reverse == true)
    {
      value = 1.0 - value;
    }
    return this->ConvertHSVtoRGB(((this->EndH - this->StartH) * value) + this->StartH, this->S, this->V, r, g, b);
  }
};




Colormap colmap;

void setup() {
  Serial.begin(115200);
  pixels.begin(); // This initializes the NeoPixel library.
}

void func1()
{
  int r, g, b;
  // 発光テスト
  for(int k = 0; k < 100; k++)
  {
    colmap.GetColor((double)k / 100, &r, &g, &b);
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println("");

    //delay(100);
    for(int i = 0; i < NUMPIXELS; i++)
    {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }

  return;
}



void func2()
{
  int r, g, b;

  float f = 0.2;
  float X = 0.3;
  float dx = 0.02; // [mm]
  float power = 0.05; // max: 1.0

  for(int i = 0; i < 100; i++)
  {
    for(int i = 0; i < NUMPIXELS; i++)
    {
      float t = (float)millis() / 1000.0f;
      float x = i * dx;
      float theta = 2.0 * 3.14 * (f * t + x / X);
      float y = (sin(theta) + 1.0f) / 2.0f;
      colmap.GetColor(y, &r, &g, &b);
      r = (int)((float)r * power); // power save
      g = (int)((float)g * power);
      b = (int)((float)b * power);
      Serial.print(r);
      Serial.print(",");
      Serial.print(g);
      Serial.print(",");
      Serial.print(b);
      Serial.println("");

      //delay(100);

      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(r, g, b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
    }
  }
  return;
}

// 点灯が流れるように発光する
void func3()
{
  int _max = 20;
  int r = random(0, _max), g = random(0, _max), b = random(0, _max);


  for(int i = 0; i < NUMPIXELS; i++)
  {
    int index = i;
    int index_pre = i - 1;
    if(i == 0) index_pre = NUMPIXELS - 1;
    pixels.setPixelColor(index, pixels.Color(r, g, b)); // Moderately bright green color.
    pixels.setPixelColor(index_pre, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(random(1, 100));
  }

  r = random(0, _max), g = random(0, _max), b = random(0, _max);
  for(int i = NUMPIXELS; i > 0; i--)
  {
    int index = i;
    int index_pre = i + 1;
    if(i == NUMPIXELS - 1) index_pre = 0;
    pixels.setPixelColor(index, pixels.Color(r, g, b)); // Moderately bright green color.
    pixels.setPixelColor(index_pre, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(random(1, 100));
  }
  return;
}



// ホテル発光
void func4()
{
  int _max = 20;
  int r = random(0, _max), g = random(0, _max), b = random(0, _max);
  int index = random(0, NUMPIXELS);
  long t;
  long term = 1500;


  for(int k = 0; k < random(1, 10); k++)
  {
    t = millis();
    long tw = 0l;
    while(tw < term)
    {
      float scale = sin(3.14 * (float)tw / (float)term);
      int _r = (int)((float)r * scale);
      int _g = (int)((float)g * scale);
      int _b = (int)((float)b * scale);
      pixels.setPixelColor(index, pixels.Color(_r, _g, _b)); // Moderately bright green color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      tw = millis() - t;
    }
    delay(200);
  }

  int _delay = 2000;
  delay(random(_delay, _delay * 4));
  return;
}




void loop() 
{
  func3();
}









