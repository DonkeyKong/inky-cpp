#pragma once

#include <string>
#include <vector>

class SPIDevice
{
public:
  SPIDevice(std::string spiDeviceName = "/dev/spidev0.0", uint32_t maxBusSpeedHz = 488000, uint32_t maxTransferSizeBytes = 4096 );
  virtual ~SPIDevice();
protected: 
  // Write the data in buf to the provided address 
  int writeSPI(const std::vector<uint8_t>& buf = std::vector<uint8_t>(), uint16_t delayUs = 0);
  // Write the data in buf to the provided address
  int writeSPI(const uint8_t* buf, size_t len, uint16_t delayUs = 0);
  // Poke the provided address, wait, then fill buf with the returned data
  int readSPI(std::vector<uint8_t>& buf, uint16_t delayUs = 0);
  // Poke the provided address, wait, then read len bytes into buf
  int readSPI(uint8_t* buf, size_t len, uint16_t delayUs = 0);
private:
  int spiFile_ = -1;
  uint8_t spiMode_ = 0;
  uint8_t bitsPerWord_ = 8;
  uint32_t maxBusSpeedHz_ = 488000;
  uint32_t maxTransferSizeBytes_ = 4096;
};
