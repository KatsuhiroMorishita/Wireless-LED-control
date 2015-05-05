/* program name: chikaken_tokyo201412                   */
/* author:  Katsuhiro MORISHITA                         */
/* purpose: 温度に合わせて、テープLEDの発光色を変える。        */
/* create:  2014-12-08                                  */
/* license:  MIT                                        */
const char pattern_code_header = 0x3f;

Stream* xbee_serial = &Serial1;


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



void light(int r, int g, int b, int unit_id)
{
  // send header
  xbee_serial->write(pattern_code_header);
  delay(5);

  // send RGB pattern
  int limit = 127;
  if(r > limit)
    r = limit;
  if(r < 0)
    r = 0;
  xbee_serial->write(r | 0x80);
  if(g > limit)
    g = limit;
  if(g < 0)
    g = 0;
  xbee_serial->write(g | 0x80);
  if(b > limit)
    b = limit;
  if(b < 0)
    b = 0;
  xbee_serial->write(b | 0x80);

  // send id
  xbee_serial->write(unit_id | 0x80);
  return;
}


void get_temperature_diff(int* t0, int* t1, int* t2, int* t3)
{
  int temp_out = analogRead(A4);
  *t0 = temp_out - analogRead(A0);
  *t1 = temp_out - analogRead(A1);
  *t2 = temp_out - analogRead(A2);
  *t3 = temp_out - analogRead(A3);

  int th = 4;
  if(*t0 < th) *t0 = 0;
  if(*t1 < th) *t1 = 0;
  if(*t2 < th) *t2 = 0;
  if(*t3 < th) *t3 = 0;


  Serial.print(*t0);
  Serial.print(",");
  Serial.print(*t1);
  Serial.print(",");
  Serial.print(*t2);
  Serial.print(",");
  Serial.print(*t3);
  Serial.print(",");
  Serial.print(temp_out);
  Serial.println("");
  return;
}







Colormap colmap;

void setup()
{
  // 通信のセットアップ
  Serial.begin(115200);
  Serial1.begin(19200);

  // 発光テスト
  int r, g, b;
  for(int id = 1; id <= 4; id++)
  {
    for(int i = 0; i < 100; i++)
    {
      colmap.GetColor((double)i / 100, &r, &g, &b);
      Serial.print(r);
      Serial.print(",");
      Serial.print(g);
      Serial.print(",");
      Serial.print(b);
      Serial.println("");

      delay(100);
      light(r, g, b, id);
    }
  }

  int dummy0, dummy1, dummy2, dummy3;
  for(int i = 0; i < 30; i++)
  {
    get_temperature_diff(&dummy0, &dummy1, &dummy2, &dummy3);
    delay(500);
  }
}


void loop()
{
  int t0, t1, t2, t3;
  int diff_max = 170;
  get_temperature_diff(&t0, &t1, &t2, &t3);

  int r, g, b;
  colmap.GetColor((double)t0 / diff_max, &r, &g, &b);
  light(r, g, b, 1);
  colmap.GetColor((double)t1 / diff_max, &r, &g, &b);
  light(r, g, b, 2);
  colmap.GetColor((double)t2 / diff_max, &r, &g, &b);
  light(r, g, b, 3);
  colmap.GetColor((double)t3 / diff_max, &r, &g, &b);
  light(r, g, b, 4);

  delay(5);

}







