#include "Gpio.hpp"

#include <fmt/format.h>

#include <memory>
#include <stdexcept>

#ifndef SIMULATE_PI_HARDWARE
#include <linux/gpio.h>
#include <fcntl.h> // provides open()
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h> // provides close()
#endif

constexpr const char* GpioConsumerName = "InkyFrame";

#ifdef SIMULATE_PI_HARDWARE
// Fake line object for non-pi hardware
struct Gpio::Line
{
  sigslot::signal<int, LineTransition, std::chrono::steady_clock::time_point> signal_;
  Line(std::string gpioDevice, int line, LineMode mode, LineBias bias = LineBias::Off, int gpioFd = -1)  { }
  void changeLineMode(LineMode mode, LineBias bias = LineBias::Off) { }
  sigslot::connection subscribe(InputChangedHandler handler) { return signal_.connect(handler); }
  bool read() { return false; }
  void write(bool val) { }
  void processEvents() { }
};
#else
struct Gpio::Line
{
  std::string gpioDevice_;
  int line_;
  int fd_;
  LineMode mode_;
  LineBias bias_;
  sigslot::signal<int, LineTransition, std::chrono::steady_clock::time_point> signal_;
  
  static uint64_t getFlags(LineMode mode, LineBias bias)
  {
    uint64_t flags = 0;
    if (mode == LineMode::Input)  flags |= GPIO_V2_LINE_FLAG_INPUT;
    if (mode == LineMode::Output) flags |= GPIO_V2_LINE_FLAG_OUTPUT;
    if (bias == LineBias::Off)    flags |= GPIO_V2_LINE_FLAG_BIAS_DISABLED;
    if (bias == LineBias::PullDown)   flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN;
    if (bias == LineBias::PullUp)     flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
    if (mode == LineMode::Input)  flags |= GPIO_V2_LINE_FLAG_EDGE_RISING;
    if (mode == LineMode::Input)  flags |= GPIO_V2_LINE_FLAG_EDGE_FALLING;
    return flags;
  }

  static void throwIfStatusError(int status, const std::string& msg = "Unknown error")
  {
    if (status != 0)
    {
      throw std::runtime_error(fmt::format("{} ({})", msg, strerror(errno)));
    }
  }

  Line(std::string gpioDevice, int line, LineMode mode, LineBias bias = LineBias::Off, int gpioFd = -1):
    gpioDevice_(gpioDevice),
    line_(line),
    fd_(-1)
  {
    // Check if we were provided a valid gpio accessor and if not, create one.
    bool closeGpioFd = false;
    if (gpioFd == -1)
    {
      closeGpioFd = true;
      gpioFd = open(gpioDevice.c_str(), 0);
      if (gpioFd == -1) 
      {
        throw std::runtime_error("Cannot open the specified gpio device!");
      }
    }

    // Request the line
    struct gpio_v2_line_request lineReq;
    memset(&lineReq, 0, sizeof(lineReq));
    strncpy(lineReq.consumer, GpioConsumerName, GPIO_MAX_NAME_SIZE);
    lineReq.offsets[0] = line_;
    lineReq.num_lines = 1;
    lineReq.config.flags = getFlags(mode, bias);

    int status = ioctl(gpioFd, GPIO_V2_GET_LINE_IOCTL, &lineReq);
    throwIfStatusError(status, "Cannot get line!");

    // Close the global accessor if we need to
    if (closeGpioFd)
    {
      close(gpioFd);
    }

    fd_ = lineReq.fd;

    //changeLineMode(mode, bias);
  }

  void changeLineMode(LineMode mode, LineBias bias = LineBias::Off)
  {
    if (fd_ == -1) throw std::runtime_error("Line is not open!");

    // Create a line config to submit
    struct gpio_v2_line_config lineConfig;
    memset(&lineConfig, 0, sizeof(lineConfig));
    lineConfig.flags = getFlags(mode, bias);

    // Configure the line
    int status = ioctl(fd_, GPIO_V2_LINE_SET_CONFIG_IOCTL, &lineConfig);
    throwIfStatusError(status, "Cannot configure line!");

    mode_ = mode;
    bias_ = bias;
  }

  sigslot::connection subscribe(InputChangedHandler handler)
  {
    return signal_.connect(handler);
  }

  bool read()
  {
    if (fd_ == -1) throw std::runtime_error("Line is not open!");
    if (mode_ != LineMode::Output) throw std::runtime_error("Cannot read to non-input lines!");

    int status;
    struct gpio_v2_line_values lv;
    lv.mask = 0x01;
    lv.bits = 0x00;

    status = ioctl(fd_, GPIO_V2_LINE_GET_VALUES_IOCTL, &lv);
    throwIfStatusError(status, "Error reading line!");
    return lv.bits != 0;  
  }

  void write(bool val)
  {
    if (fd_ == -1) throw std::runtime_error("Line is not open!");
    if (mode_ != LineMode::Output) throw std::runtime_error("Cannot write to non-output lines!");

    int status;
    struct gpio_v2_line_values lv;
    lv.mask = 0x01;
    lv.bits = val ? 0x01 : 0x00;

    status = ioctl(fd_, GPIO_V2_LINE_SET_VALUES_IOCTL, &lv);
    throwIfStatusError(status, "Error writing line!");
  }

  void processEvents()
  {
    int ret = 1;
    ssize_t bytes;
    fd_set set;
 
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    FD_ZERO(&set);    /* clear the set */
    FD_SET(fd_, &set); /* add our file descriptor to the set */
    
    while (ret > 0)
    {
      ret = select(fd_ + 1, &set, NULL, NULL, &timeout);

      if (ret > 0)
      {
        struct gpio_v2_line_event event;
        ret = ::read(fd_, &event, sizeof(event));
        if (ret == sizeof(event)) 
        {
          std::chrono::steady_clock::time_point timestamp(std::chrono::nanoseconds(event.timestamp_ns));
          if (event.id == GPIO_V2_LINE_EVENT_RISING_EDGE)
          {
            signal_(line_, LineTransition::RisingEdge, timestamp);
          }
          else if (event.id == GPIO_V2_LINE_EVENT_FALLING_EDGE)
          {
            signal_(line_, LineTransition::FallingEdge, timestamp);
          }
        }
      }
    }

    FD_ZERO(&set); /* clear the set */
  }

  ~Line()
  {
    if (fd_ != -1)
    {
      close(fd_);
    }
  }
};
#endif

Gpio::Gpio(std::string gpioDevice):
  stopThread_(false),
  gpioDevice_(gpioDevice)
{
  thread_ = std::make_unique<std::thread>([&]()
  {
    while (!stopThread_)
    {
      {
        std::lock_guard lock(mutex_);
        // Iterate over all the exported lines looking for events to raise
        for (const auto& [pin, line] : lines_)
        {
          line->processEvents();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(16)); // throttle events
    }
  });
}

void Gpio::setupLine(int line, LineMode mode, LineBias bias)
{
  std::lock_guard lock(mutex_);
  if (!(lines_.count(line) > 0))
  {
    lines_.emplace(line, std::make_unique<Line>(gpioDevice_, line, mode, bias));
  }
  else
  {
    lines_[line]->changeLineMode(mode, bias);
  }
}

void Gpio::releaseLine(int line)
{
  std::lock_guard lock(mutex_);
  lines_.erase(line);
}

// Get a line's active status as a bool, true if the line is active, false otherwise.
bool Gpio::read(int line)
{
  std::lock_guard lock(mutex_);
  return lines_[line]->read();
}

// Set a line active or inactive
void Gpio::write(int line, bool value)
{
  std::lock_guard lock(mutex_);
  lines_[line]->write(value);
}

sigslot::connection Gpio::subscribe(int line, InputChangedHandler handler)
{
  std::lock_guard lock(mutex_);
  return lines_[line]->subscribe(handler);
}

Gpio::~Gpio()
{
  stopThread_ = true;
  if (thread_ && thread_->joinable())
  {
    thread_->join();
    thread_ = nullptr;
  }
}

