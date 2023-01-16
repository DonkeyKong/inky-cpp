#pragma once

#include <string>
#include <vector>

class I2CDevice
{
public:
  I2CDevice(uint8_t deviceId, std::string i2cDeviceName = "/dev/i2c-1");
  virtual ~I2CDevice();
protected: 
  // Write the data in buf to the provided address 
  bool writeI2C(uint16_t addr, const std::vector<uint8_t>& buf = std::vector<uint8_t>());
  // Write the data in buf to the provided address
  bool writeI2C(uint16_t addr, const uint8_t* buf, size_t len);
  // Poke the provided address, wait, then fill buf with the returned data
  bool readI2C(uint16_t addr, std::vector<uint8_t>& buf, double delayMs = 8.0);
  // Poke the provided address, wait, then read len bytes into buf
  bool readI2C(uint16_t addr, uint8_t* buf, size_t len, double delayMs = 8.0);
  // Wait the specified number of milliseconds
  void delay(double milliseconds);
private:
  int i2cFile_;
};
