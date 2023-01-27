#pragma once

#include "Image.hpp"
#include <vector>

class Inky
{
public:

  static std::unique_ptr<Inky> Create();

  enum class ColorCapability : uint8_t
  {
    //Invalid = 0,
    BlackWhite = 1,
    BlackWhiteRed = 2,
    BlackWhiteYellow = 3,
    //Invalid = 4,
    SevenColor = 5
  };

  enum class DisplayVariant : uint8_t
  {
      //Invalid = 0,
      Red_pHAT_High_Temp = 1,
      Yellow_wHAT = 2,
      Black_wHAT = 3,
      Black_pHAT = 4,
      Yellow_pHAT = 5,
      Red_wHAT = 6,
      Red_wHAT_High_Temp = 7,
      Red_wHAT_v2 = 8,
      //Invalid = 9,
      Black_pHAT_SSD1608 = 10,
      Red_pHAT_SSD1608 = 11,
      Yellow_pHAT_SSD1608 = 12,
      //Invalid = 13,
      Seven_Colour_UC8159 = 14,
      Seven_Colour_640x400_UC8159 = 15,
      Seven_Colour_640x400_UC8159_v2 = 16,
      Black_wHAT_SSD1683 = 17,
      Red_wHAT_SSD1683 = 18,
      Yellow_wHAT_SSD1683 = 19
  };
  struct DisplayInfo
  {
    uint16_t width;
    uint16_t height;
    ColorCapability colorCapability;
    uint8_t pcbVariant;
    DisplayVariant displayVariant;
    std::string writeTime;
  };

  virtual ~Inky() = 0;
  virtual void setImage(const Image& image) = 0;
  virtual void setBorder(IndexedColor color) = 0;
  virtual void show() = 0;
  virtual const DisplayInfo& info() const = 0;
};