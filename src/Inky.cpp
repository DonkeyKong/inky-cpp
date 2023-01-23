#include "Inky.hpp"
#ifdef PI_HOST
#include "minimal_gpio.h"
#endif
#include <algorithm>

static const uint8_t InkyEEPROMI2CDeviceId = 0x50;

Inky::Inky() : I2CDevice(InkyEEPROMI2CDeviceId)
{
  readEeprom();
}

Inky::~Inky()
{

}

static uint16_t read16(uint8_t* buf)
{
  return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

void Inky::readEeprom()
{
  // eeprom format is python struct '<HHBBB22p'
  // width, height, color, pcb_variant, display_variant, write_time
  uint8_t rom[30];
  readI2C(0, rom, 29);

  width_ = read16(rom);
  height_ = read16(rom+2);
  colorCapability_ = (ColorCapability)rom[4];
  pcbVariant_ = rom[5];
  displayVariant_ = (DisplayVariant)rom[6];

  // Get the length of the string and null terminate it
  // The data could lie about length, so cap it to 21
  uint8_t len = std::min(rom[7],(uint8_t)21); 
  rom[8+len] = '\0';

  // Consruct a std::string out of our newly minted cstr
  writeTime_ = std::string((const char*)rom+8);
}

uint16_t Inky::width() const 
{
  return width_;
}

uint16_t Inky::height() const
{
  return height_;
}

Inky::ColorCapability Inky::colorCapability() const
{
  return colorCapability_;
}

uint8_t Inky::pcbVariant() const
{
  return pcbVariant_;
}

Inky::DisplayVariant Inky::displayVariant() const
{
  return displayVariant_;
}

std::string Inky::writeTime() const
{
  return writeTime_;
}

void Inky::setImage(Image image)
{

}

void Inky::setBorder(InkyColor inky)
{

}

void Inky::show()
{
  
}
