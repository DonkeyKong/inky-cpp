#pragma once

#include "Image.hpp"
#include "I2CDevice.hpp"

class Inky : public I2CDevice
{
public:

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

  Inky();
  ~Inky();
  void setImage(Image image);
  void setBorder(InkyColor color);
  void show();

  uint16_t width() const;
  uint16_t height() const;
  ColorCapability colorCapability() const;
  uint8_t pcbVariant() const;
  DisplayVariant displayVariant() const;
  std::string writeTime() const;

private:
  uint16_t width_;
  uint16_t height_;
  ColorCapability colorCapability_;
  uint8_t pcbVariant_;
  DisplayVariant displayVariant_;
  std::string writeTime_;
  void readEeprom();
};
