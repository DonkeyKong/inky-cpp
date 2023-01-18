#include "Color.hpp"

#include <cmath>
#include <algorithm>
#include <random>

RGBAColor HSVColor::toRGB()
{
  float r, g, b;
  int range = (int)std::floor(H / 60.0f);
  float c = V * S;
  float x = c * (1 - std::abs(fmod(H / 60.0f, 2.0f) - 1));
  float m = V - c;

  switch (range)
  {
  case 0:
    r = (c + m);
    g = (x + m);
    b = m;
    break;
  case 1:
    r = (x + m);
    g = (c + m);
    b = m;
    break;
  case 2:
    r = m;
    g = (c + m);
    b = (x + m);
    break;
  case 3:
    r = m;
    g = (x + m);
    b = (c + m);
    break;
  case 4:
    r = (x + m);
    g = m;
    b = (c + m);
    break;
  default: // case 5:
    r = (c + m);
    g = m;
    b = (x + m);
    break;
  }

  return 
  {
    (uint8_t) std::clamp((r * 255.0f), 0.0f, 255.0f),
    (uint8_t) std::clamp((g * 255.0f), 0.0f, 255.0f),
    (uint8_t) std::clamp((b * 255.0f), 0.0f, 255.0f),
    (uint8_t) std::clamp((A * 255.0f), 0.0f, 255.0f)
  };
}

HSVColor RGBAColor::toHSV() const
{
    HSVColor hsv;

    float r = std::clamp(R / 255.0f, 0.0f, 1.0f);
    float g = std::clamp(G / 255.0f, 0.0f, 1.0f);
    float b = std::clamp(B / 255.0f, 0.0f, 1.0f);
    hsv.A = std::clamp(A / 255.0f, 0.0f, 1.0f);

    float min = std::min(r, std::min(g, b));
    float max = std::max(r, std::max(g, b));
    float delta = max - min;

    hsv.V = max;
    hsv.S = (max > 1e-3) ? (delta / max) : 0;
    

    if (delta == 0) 
    {
        hsv.H = 0;
    }
    else 
    {
        if (r == max) 
            hsv.H = (g - b) / delta;
        else if (g == max)
            hsv.H = 2 + (b - r) / delta;
        else if (b == max)
            hsv.H = 4 + (r - g) / delta;

        hsv.H *= 60;
        hsv.H = fmod(hsv.H + 360, 360);
    }
    return hsv;
}

uint8_t RGBAColor::getBrightestChannel() const
{
  return std::max(R, std::max(G, B));
}

uint8_t RGBAColor::getGrayValue() const
{
  return (uint8_t)(0.299f * (float)R + 0.587f * (float)G + 0.114f * (float)B);
}
