#include "Inky.hpp"
#ifndef SIMULATE_PI_HARDWARE
#include "minimal_gpio.h"
#endif
#include <fmt/format.h>
#include <chrono>
#include <algorithm>
#include <stdexcept>

//#define DEBUG_SPI
#ifdef DEBUG_SPI
#include <magic_enum.hpp>
#include <iostream>
using namespace magic_enum::ostream_operators;
#endif

static const uint8_t InkyEEPROMI2CDeviceId = 0x50;
static const std::string InkySPIDevice = "/dev/spidev0.0";
static const int InkySPIDeviceSpeedHz = 10000000;
static const int InkyDcGPIO = 22;
static const int InkyResetGPIO = 27;
static const int InkyBusyGPIO = 17;

// Constants for SSD1608 driver IC
enum class Inky::InkyCommand : uint8_t
{
  DRIVER_CONTROL = 0x01,
  GATE_VOLTAGE = 0x03,
  SOURCE_VOLTAGE = 0x04,
  DISPLAY_CONTROL = 0x07,
  NON_OVERLAP = 0x0B,
  BOOSTER_SOFT_START = 0x0C,
  GATE_SCAN_START = 0x0F,
  DEEP_SLEEP = 0x10,
  DATA_MODE = 0x11,
  SW_RESET = 0x12,
  TEMP_WRITE = 0x1A,
  TEMP_READ = 0x1B,
  TEMP_CONTROL = 0x18,
  TEMP_LOAD = 0x1A,
  MASTER_ACTIVATE = 0x20,
  DISP_CTRL1 = 0x21,
  DISP_CTRL2 = 0x22,
  WRITE_RAM = 0x24,
  WRITE_ALTRAM = 0x26,
  READ_RAM = 0x25,
  VCOM_SENSE = 0x2B,
  VCOM_DURATION = 0x2C,
  WRITE_VCOM = 0x2C,
  READ_OTP = 0x2D,
  WRITE_LUT = 0x32,
  WRITE_DUMMY = 0x3A,
  WRITE_GATELINE = 0x3B,
  WRITE_BORDER = 0x3C,
  SET_RAMXPOS = 0x44,
  SET_RAMYPOS = 0x45,
  SET_RAMXCOUNT = 0x4E,
  SET_RAMYCOUNT = 0x4F,
  NOP = 0xFF,
};

static const std::vector<uint8_t> BlackLUT 
{
  0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
  0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
  0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static const std::vector<uint8_t> RedLUT 
{
  0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
  0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
  0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static const std::vector<uint8_t> YellowLUT 
{
  0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
  0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
  0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static uint16_t read16(const uint8_t* buf)
{
  return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

static void write16(uint16_t val, uint8_t* buf)
{
  buf[0] = (uint8_t) val;
  buf[1] = (uint8_t) (val >> 8);
}

Inky::Inky() : I2CDevice(InkyEEPROMI2CDeviceId), 
               SPIDevice(InkySPIDevice, InkySPIDeviceSpeedHz)
{
  readEeprom();
  border_ = InkyColor::White;

  buf_ = Image(width_, height_, (ImageFormat)colorCapability_);

  if (displayVariant_ != DisplayVariant::Black_wHAT_SSD1683 &&
      displayVariant_ != DisplayVariant::Red_wHAT_SSD1683 &&
      displayVariant_ != DisplayVariant::Yellow_wHAT_SSD1683)
  {
    throw std::runtime_error("Unsupported Inky display type!!");
  }

  #ifndef SIMULATE_PI_HARDWARE
  // Setup the GPIO pins
  if (gpioInitialise() < 0)
  {
      throw std::runtime_error("Failed to setup GPIO\n");
  }
  gpioSetMode(InkyDcGPIO, PI_OUTPUT);
  gpioSetPullUpDown(InkyDcGPIO, PI_PUD_OFF);
  gpioWrite(InkyDcGPIO, 0);
  gpioSetMode(InkyResetGPIO, PI_OUTPUT);
  gpioSetPullUpDown(InkyResetGPIO, PI_PUD_OFF);
  gpioWrite(InkyResetGPIO, 1);
  gpioSetMode(InkyBusyGPIO, PI_INPUT);
  gpioSetPullUpDown(InkyBusyGPIO, PI_PUD_OFF);
  #endif
}

Inky::~Inky()
{

}

void Inky::reset()
{
  #ifndef SIMULATE_PI_HARDWARE
  // Perform a hardware reset
  gpioWrite(InkyResetGPIO, 0);
  delay(500);
  gpioWrite(InkyResetGPIO, 1);
  delay(500);
  #endif
  sendCommand(InkyCommand::SW_RESET);
  delay(1000);
  waitForBusy();
}

void Inky::sendCommand(InkyCommand command)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 0);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void Inky::sendCommand(InkyCommand command, uint8_t param)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 0);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
  sendByte(param);
}

void Inky::sendCommand(InkyCommand command, const std::vector<uint8_t>& params)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 0);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
  sendBuffer(params);
}

void Inky::sendBuffer(const uint8_t* buffer, int len)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 1);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Sent buffer len " << len << " ret: " << 
  #endif
  writeSPI(buffer, len);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void Inky::sendBuffer(const std::vector<uint8_t>& buffer)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 1);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Sent buffer len " << buffer.size() << " ret: " << 
  #endif
  writeSPI(buffer);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void Inky::sendByte(uint8_t data)
{
  #ifndef SIMULATE_PI_HARDWARE
  gpioWrite(InkyDcGPIO, 1);
  #endif
  #ifdef DEBUG_SPI
  std::cout << "Sent byte ret: " << 
  #endif
  writeSPI(&data, 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void Inky::waitForBusy(int timeoutMs)
{
  #ifndef SIMULATE_PI_HARDWARE
  int i = 0;
  while (gpioRead(InkyBusyGPIO) != 0)
  {
    delay(10);
    ++i;
    if (i*10 > timeoutMs)
    {
      throw std::runtime_error("Timed out while wating for display to finish an operation.");
    }
  }
  #endif
}

void Inky::readEeprom()
{
  // eeprom format is python struct '<HHBBB22p'
  // width, height, color, pcb_variant, display_variant, write_time
  uint8_t rom[30];
  readI2C(0, rom, 29);

  // Some fake data on non-pi platforms
  #ifdef SIMULATE_PI_HARDWARE
  write16(400, rom);
  write16(300, rom+2);
  rom[4] = (uint8_t)ColorCapability::BlackWhiteRed;
  rom[5] = 12;
  rom[6] = (uint8_t)DisplayVariant::Red_wHAT_SSD1683;
  std::string lastWrite = "2022-09-02 11:54:06.4";
  rom[7] = (uint8_t) lastWrite.length();
  memcpy(rom+8, lastWrite.data(), lastWrite.length());
  #endif

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

void Inky::setImage(const Image& image)
{
  buf_ = image;
  buf_.scale(width_, height_, {ImageScaleMode::Fill});
  buf_.convert((ImageFormat) colorCapability_, {.ditherMode = DitherMode::Diffusion, .ditherAccuracy = 0.75f});
}

void Inky::setBorder(InkyColor inky)
{
  border_ = inky;
}

void Inky::generatePackedPlane(std::vector<uint8_t>& packed, InkyColor color)
{
  int size = (buf_.width()*buf_.height());
  int wholeBytes = size/8;
  int packedSize = size/8 + ((size%8 != 0) ? 1 : 0);
  packed.resize(packedSize);
  const InkyColor* inkyData = (const InkyColor*)buf_.data();
  for (int i = 0; i < wholeBytes; ++i)
  {
    packed[i] = ((inkyData[i*8+0] == color) ? (uint8_t)0b10000000 : (uint8_t)0) | 
                ((inkyData[i*8+1] == color) ? (uint8_t)0b01000000 : (uint8_t)0) |
                ((inkyData[i*8+2] == color) ? (uint8_t)0b00100000 : (uint8_t)0) |
                ((inkyData[i*8+3] == color) ? (uint8_t)0b00010000 : (uint8_t)0) |
                ((inkyData[i*8+4] == color) ? (uint8_t)0b00001000 : (uint8_t)0) |
                ((inkyData[i*8+5] == color) ? (uint8_t)0b00000100 : (uint8_t)0) |
                ((inkyData[i*8+6] == color) ? (uint8_t)0b00000010 : (uint8_t)0) |
                ((inkyData[i*8+7] == color) ? (uint8_t)0b00000001 : (uint8_t)0);
  }

  // Get that sneaky final byte and write to it
  if (packedSize > wholeBytes)
  {
    packed[wholeBytes] = 0;
    if (wholeBytes*8+0 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+0] == color) ? (uint8_t)0b00000001 : (uint8_t)0); 
    if (wholeBytes*8+1 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+1] == color) ? (uint8_t)0b00000010 : (uint8_t)0);
    if (wholeBytes*8+2 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+2] == color) ? (uint8_t)0b00000100 : (uint8_t)0);
    if (wholeBytes*8+3 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+3] == color) ? (uint8_t)0b00001000 : (uint8_t)0);
    if (wholeBytes*8+4 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+4] == color) ? (uint8_t)0b00010000 : (uint8_t)0);
    if (wholeBytes*8+5 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+5] == color) ? (uint8_t)0b00100000 : (uint8_t)0);
    if (wholeBytes*8+6 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+6] == color) ? (uint8_t)0b01000000 : (uint8_t)0);
    if (wholeBytes*8+7 < size) packed[wholeBytes] |= ((inkyData[wholeBytes*8+7] == color) ? (uint8_t)0b10000000 : (uint8_t)0);
  }
}

static inline int64_t millisecondsSinceEpoch()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void Inky::show()
{
  reset();

  sendCommand(InkyCommand::DRIVER_CONTROL, {(uint8_t)(height_ - 1), (uint8_t)((height_ - 1) >> 8), 0x00});
  // Set dummy line period
  sendCommand(InkyCommand::WRITE_DUMMY, 0x1B);
  // Set Line Width
  sendCommand(InkyCommand::WRITE_GATELINE, 0x0B);
  // Data entry squence (scan direction leftward and downward)
  sendCommand(InkyCommand::DATA_MODE, 0x03);
  // Set ram X start and end position
  sendCommand(InkyCommand::SET_RAMXPOS, {0x00, (uint8_t)((width_ / 8) - 1)});
  // Set ram Y start and end position
  sendCommand(InkyCommand::SET_RAMYPOS, {0x00, 0x00, (uint8_t)(height_ - 1), (uint8_t)((height_ - 1) >> 8)});
  // VCOM Voltage
  sendCommand(InkyCommand::WRITE_VCOM, 0x70);
  // Write LUT DATA
  // sendCommand(InkyCommand::WRITE_LUT, self._luts[self.lut])

  if (border_ == InkyColor::Black)
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000000);
    // GS Transition + Waveform 00 + GSA 0 + GSB 0
  }  
  else if (border_ == InkyColor::Red && colorCapability_ == ColorCapability::BlackWhiteRed)
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000110);
    // GS Transition + Waveform 01 + GSA 1 + GSB 0
  }
  else if (border_ == InkyColor::Red && colorCapability_ == ColorCapability::BlackWhiteYellow)
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00001111);
    // GS Transition + Waveform 11 + GSA 1 + GSB 1
  }
  else if (border_ == InkyColor::White)
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000001);
    // GS Transition + Waveform 00 + GSA 0 + GSB 1
  }

  // Set RAM address to 0, 0
  sendCommand(InkyCommand::SET_RAMXCOUNT, 0x00);
  sendCommand(InkyCommand::SET_RAMYCOUNT, {0x00, 0x00});

  // Write the images to display RAM
  generatePackedPlane(whitePlane_, InkyColor::White);

  sendCommand(InkyCommand::WRITE_RAM, whitePlane_);
  if (colorCapability_ == ColorCapability::BlackWhiteRed)
  {
    generatePackedPlane(colorPlane_, InkyColor::Red);
    sendCommand(InkyCommand::WRITE_ALTRAM, colorPlane_);
  }
  else if (colorCapability_ == ColorCapability::BlackWhiteYellow)
  {
    generatePackedPlane(colorPlane_, InkyColor::Yellow);
    sendCommand(InkyCommand::WRITE_ALTRAM, colorPlane_);
  }
  
  waitForBusy();
  sendCommand(InkyCommand::MASTER_ACTIVATE);

  #ifdef SIMULATE_PI_HARDWARE
  buf_.writePng(fmt::format("Inky_{}.png", millisecondsSinceEpoch()));
  #endif
}
