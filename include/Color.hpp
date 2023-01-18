#pragma once

#include <stdint.h>

enum class InkyColor : uint8_t
{
  White = 0,
  Black = 1,
  Red = 2,
  Yellow = 2
};

#pragma pack(push, 1)
struct RGBAColor;
struct HSVColor;
struct HSVColor
{
  float H = 0.0f;
  float S = 0.0f;
  float V = 0.0f;
  float A = 1.0f;

  RGBAColor toRGB();
};

struct RGBAColor
{
  uint8_t R = 0;
  uint8_t G = 0;
  uint8_t B = 0;
  uint8_t A = 255;

  HSVColor toHSV() const;
  uint8_t getBrightestChannel() const;
  uint8_t getGrayValue() const;
};
#pragma pack(pop)
