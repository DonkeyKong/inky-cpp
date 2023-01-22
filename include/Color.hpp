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
struct LabColor;
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
  LabColor toLab() const;
  uint8_t getBrightestChannel() const;
  uint8_t getGrayValue() const;
};

struct XYZColor
{
  float X = 0;
  float Y = 0;
  float Z = 0;
};

struct LabColor
{
  float L = 0;
  float a = 0;
  float b = 0;

  LabColor operator+ (const LabColor& c)  const
  {
    return {L + c.L, a + c.a, b + c.b};
  }

  LabColor operator* (const LabColor& c)  const
  {
    return {L * c.L, a * c.a, b * c.b};
  }

  void operator+= (const LabColor& c)
  {
    L += c.L;
    a += c.a;
    b += c.b;
  }

  LabColor operator- (const LabColor& c) const
  {
    return {L - c.L, a - c.a, b - c.b};
  }

  LabColor operator* (float c) const
  {
    return {c*L, c*a, c*b};
  }

  friend LabColor operator*(float c, const LabColor& rhs)
  {
      return {c*rhs.L, c*rhs.a, c*rhs.b};
  }

  float deltaE(const LabColor& other) const;
};

#pragma pack(pop)
