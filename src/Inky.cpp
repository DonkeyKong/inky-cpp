#include "Inky.hpp"
#include "I2CDevice.hpp"
#include "SPIDevice.hpp"

#include <fmt/format.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <stdexcept>

#ifdef SIMULATE_PI_HARDWARE
#include "minimal_gpio_nop.h"
#else
#include "minimal_gpio.h"
#endif

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
enum class InkyCommand : uint8_t
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

Inky::~Inky() {}

class InkyBase : public Inky, public SPIDevice
{
protected:

  DisplayInfo info_;
  IndexedColor border_;
  Image buf_;
  IndexedColorMap colorMap_;

  InkyBase(DisplayInfo info); 

  virtual void setImage(const Image& image) override;
  virtual void setBorder(IndexedColor color) override;
  virtual const DisplayInfo& info() const override;

  void waitForBusy(int timeoutMs = 5000);
  void sendCommand(InkyCommand command);
  void sendCommand(InkyCommand command, uint8_t param);
  void sendCommand(InkyCommand command, const std::vector<uint8_t>& params);
  static void generatePackedPlane(const Image& img, std::vector<uint8_t>& packed, IndexedColor color);
  void sendBuffer(const uint8_t* data, int len);
  void sendBuffer(const std::vector<uint8_t>& data);
  void sendByte(uint8_t data);
  static void sleep(double milliseconds);
};

InkyBase::InkyBase(DisplayInfo displayInfo) : SPIDevice(InkySPIDevice, InkySPIDeviceSpeedHz),
  info_(displayInfo)
{
  std::vector<std::tuple<ColorName,IndexedColor,RGBAColor>> displayColors;
  switch (info_.colorCapability)
  {
    case ColorCapability::BlackWhite:
      displayColors = 
      {
        {ColorName::White, 0, {255,255,255}},
        {ColorName::Black, 1, {0,0,0}}
      };
      break;
    case ColorCapability::BlackWhiteRed:
      displayColors = 
      {
        {ColorName::White, 0, {255,255,255}},
        {ColorName::Black, 1, {0,0,0}},
        {ColorName::Red, 2, {255,0,0}}
      };
      break;
    case ColorCapability::BlackWhiteYellow:
      displayColors = 
      {
        {ColorName::White, 0, {255,255,255}},
        {ColorName::Black, 1, {0,0,0}},
        {ColorName::Red, 2, {255,0,0}}
      };
      break;
    case ColorCapability::SevenColor:
      displayColors = 
        {
          {ColorName::Black, 0, {0,0,0}},
          {ColorName::White, 1, {255,255,255}},
          {ColorName::Green, 2, {0,255,0}},
          {ColorName::Blue, 3, {0,0,255}},
          {ColorName::Red, 4, {255,0,0}},
          {ColorName::Yellow, 5, {255,255,0}},
          {ColorName::Orange, 6, {255,127,0}}
        };
      break;
  }
  colorMap_ = IndexedColorMap(displayColors);
  border_ = colorMap_.toIndexedColor(ColorName::White);
  buf_ = Image(info_.width, info_.height, colorMap_);
}

void InkyBase::sendCommand(InkyCommand command)
{
  gpioWrite(InkyDcGPIO, 0);
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void InkyBase::sendCommand(InkyCommand command, uint8_t param)
{
  gpioWrite(InkyDcGPIO, 0);
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
  sendByte(param);
}

void InkyBase::sendCommand(InkyCommand command, const std::vector<uint8_t>& params)
{
  gpioWrite(InkyDcGPIO, 0);
  #ifdef DEBUG_SPI
  std::cout << "Command " << command << " ret: " << 
  #endif
  writeSPI((uint8_t*)(&command), 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
  sendBuffer(params);
}

void InkyBase::sendBuffer(const uint8_t* buffer, int len)
{
  gpioWrite(InkyDcGPIO, 1);
  #ifdef DEBUG_SPI
  std::cout << "Sent buffer len " << len << " ret: " << 
  #endif
  writeSPI(buffer, len);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void InkyBase::sendBuffer(const std::vector<uint8_t>& buffer)
{
  gpioWrite(InkyDcGPIO, 1);
  #ifdef DEBUG_SPI
  std::cout << "Sent buffer len " << buffer.size() << " ret: " << 
  #endif
  writeSPI(buffer);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void InkyBase::sendByte(uint8_t data)
{
  gpioWrite(InkyDcGPIO, 1);
  #ifdef DEBUG_SPI
  std::cout << "Sent byte ret: " << 
  #endif
  writeSPI(&data, 1);
  #ifdef DEBUG_SPI
  std::cout << std::endl;
  #endif
}

void InkyBase::waitForBusy(int timeoutMs)
{
  int i = 0;
  while (gpioRead(InkyBusyGPIO) != 0)
  {
    sleep(10);
    ++i;
    if (i*10 > timeoutMs)
    {
      throw std::runtime_error("Timed out while wating for display to finish an operation.");
    }
  }
}

const Inky::DisplayInfo& InkyBase::info() const 
{
  return info_;
}

void InkyBase::setImage(const Image& image)
{
  buf_ = image;
  buf_.scale(info_.width, info_.height, {ImageScaleMode::Fill});
  buf_.toIndexed(colorMap_, {.ditherMode = DitherMode::Diffusion, .ditherAccuracy = 0.75f});
}

void InkyBase::setBorder(IndexedColor inky)
{
  border_ = inky;
}

void InkyBase::generatePackedPlane(const Image& img, std::vector<uint8_t>& packed, IndexedColor color)
{
  int size = (img.width()*img.height());
  int wholeBytes = size/8;
  int packedSize = size/8 + ((size%8 != 0) ? 1 : 0);
  packed.resize(packedSize);
  const IndexedColor* inkyData = (const IndexedColor*)img.data();
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

void InkyBase::sleep(double milliseconds)
{
  if (milliseconds > 0.0)
  {
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(milliseconds));
  }
}

static inline int64_t millisecondsSinceEpoch()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

class SimulatedInky : public InkyBase
{
  public:
  SimulatedInky();
  virtual void show() override;
};

SimulatedInky::SimulatedInky() : InkyBase(
  {
    // Some fake data on non-pi platforms
    .width = 400,
    .height = 300,
    .colorCapability = ColorCapability::BlackWhiteRed,
    .pcbVariant = 12,
    .displayVariant = DisplayVariant::Red_wHAT_SSD1683,
    .writeTime = "2022-09-02 11:54:06.4"
  }
) {}

void SimulatedInky::show()
{
  buf_.writePng(fmt::format("Inky_{}.png", millisecondsSinceEpoch()));
}

class InkySSD1683 final : public InkyBase
{
  private: 
  std::vector<uint8_t> whitePlane;
  std::vector<uint8_t> colorPlane;
  void reset();
  public:
  InkySSD1683(DisplayInfo info);
  virtual void show() override;
};

InkySSD1683::InkySSD1683(DisplayInfo info) : InkyBase(info)
{
  if (info.displayVariant != DisplayVariant::Black_wHAT_SSD1683 &&
      info.displayVariant != DisplayVariant::Red_wHAT_SSD1683 &&
      info.displayVariant != DisplayVariant::Yellow_wHAT_SSD1683)
  {
    throw std::runtime_error("Unsupported Inky display type!!");
  }

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
}

void InkySSD1683::reset()
{
  // Perform a hardware reset
  gpioWrite(InkyResetGPIO, 0);
  sleep(500);
  gpioWrite(InkyResetGPIO, 1);
  sleep(500);
  sendCommand(InkyCommand::SW_RESET);
  sleep(1000);
  waitForBusy();
}

void InkySSD1683::show()
{
  reset();

  sendCommand(InkyCommand::DRIVER_CONTROL, {(uint8_t)(info_.height - 1), (uint8_t)((info_.height - 1) >> 8), 0x00});
  // Set dummy line period
  sendCommand(InkyCommand::WRITE_DUMMY, 0x1B);
  // Set Line Width
  sendCommand(InkyCommand::WRITE_GATELINE, 0x0B);
  // Data entry squence (scan direction leftward and downward)
  sendCommand(InkyCommand::DATA_MODE, 0x03);
  // Set ram X start and end position
  sendCommand(InkyCommand::SET_RAMXPOS, {0x00, (uint8_t)((info_.width / 8) - 1)});
  // Set ram Y start and end position
  sendCommand(InkyCommand::SET_RAMYPOS, {0x00, 0x00, (uint8_t)(info_.height - 1), (uint8_t)((info_.height - 1) >> 8)});
  // VCOM Voltage
  sendCommand(InkyCommand::WRITE_VCOM, 0x70);
  // Write LUT DATA
  // sendCommand(InkyCommand::WRITE_LUT, self._luts[self.lut])

  if (border_ == colorMap_.toIndexedColor(ColorName::Black))
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000000);
    // GS Transition + Waveform 00 + GSA 0 + GSB 0
  }  
  else if (border_ == colorMap_.toIndexedColor(ColorName::Red))
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000110);
    // GS Transition + Waveform 01 + GSA 1 + GSB 0
  }
  else if (border_ == colorMap_.toIndexedColor(ColorName::Yellow))
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00001111);
    // GS Transition + Waveform 11 + GSA 1 + GSB 1
  }
  else if (border_ == colorMap_.toIndexedColor(ColorName::White))
  {
    sendCommand(InkyCommand::WRITE_BORDER, 0b00000001);
    // GS Transition + Waveform 00 + GSA 0 + GSB 1
  }

  // Set RAM address to 0, 0
  sendCommand(InkyCommand::SET_RAMXCOUNT, 0x00);
  sendCommand(InkyCommand::SET_RAMYCOUNT, {0x00, 0x00});

  // Write the images to display RAM
  generatePackedPlane(buf_, whitePlane, colorMap_.toIndexedColor(ColorName::White));
  sendCommand(InkyCommand::WRITE_RAM, whitePlane);

  if (info_.colorCapability == ColorCapability::BlackWhiteRed)
  {
    generatePackedPlane(buf_, colorPlane, colorMap_.toIndexedColor(ColorName::Red));
    sendCommand(InkyCommand::WRITE_ALTRAM, colorPlane);
  }
  else if (info_.colorCapability == ColorCapability::BlackWhiteYellow)
  {
    generatePackedPlane(buf_, colorPlane, colorMap_.toIndexedColor(ColorName::Yellow));
    sendCommand(InkyCommand::WRITE_ALTRAM, colorPlane);
  }

  waitForBusy();
  sendCommand(InkyCommand::MASTER_ACTIVATE);
}

class InkyUC8159 final : public InkyBase
{
  private: 
  std::vector<uint8_t> packed;
  void reset();
  public:
  InkyUC8159(DisplayInfo info);
  virtual void show() override;
};

InkyUC8159::InkyUC8159(DisplayInfo info) : InkyBase(info)
{

}

void InkyUC8159::reset()
{

}

void InkyUC8159::show()
{
  
}

static uint16_t read16(const uint8_t* buf)
{
  return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

static void write16(uint16_t val, uint8_t* buf)
{
  buf[0] = (uint8_t) val;
  buf[1] = (uint8_t) (val >> 8);
}

Inky::DisplayInfo readEeprom()
{
  I2CDevice eeprom(InkyEEPROMI2CDeviceId);
  // eeprom format is python struct '<HHBBB22p'
  // width, height, color, pcb_variant, display_variant, write_time
  uint8_t rom[30];
  eeprom.readI2C(0, rom, 29);

  // Get the length of the writeTime string and null terminate it
  // The data could lie about length, so cap it to 21
  uint8_t len = std::min(rom[7],(uint8_t)21); 
  rom[8+len] = '\0';

  return
  {
    .width = read16(rom),
    .height = read16(rom+2),
    .colorCapability = (Inky::ColorCapability)rom[4],
    .pcbVariant = rom[5],
    .displayVariant = (Inky::DisplayVariant)rom[6],
    .writeTime = std::string((const char*)rom+8)
  };
}

std::unique_ptr<Inky> Inky::Create()
{
  #ifdef SIMULATE_PI_HARDWARE
  return std::make_unique<SimulatedInky>();
  #else
  auto displayInfo = readEeprom();
  switch (displayInfo.displayVariant)
  {
    case DisplayVariant::Black_wHAT_SSD1683:
    case DisplayVariant::Red_wHAT_SSD1683:
    case DisplayVariant::Yellow_wHAT_SSD1683:
      return std::make_unique<InkySSD1683>(displayInfo);
    // case DisplayVariant::Seven_Colour_UC8159:
    // case DisplayVariant::Seven_Colour_640x400_UC8159:
    // case DisplayVariant::Seven_Colour_640x400_UC8159_v2:
    //   return std::make_unique<InkyUC8159>(displayInfo);
  }
  throw std::runtime_error("The connected Inky is unsupported!");
  #endif
}