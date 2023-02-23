#pragma once

#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <functional>

#include <sigslot/signal.hpp>

class Gpio
{
public:

  enum class LineMode
  {
    Input,
    Output
  };

  enum class LineBias
  {
    Off,
    PullDown,
    PullUp
  };

  enum class LineTransition
  {
    RisingEdge,
    FallingEdge
  };

  typedef std::function<void(int, LineTransition, std::chrono::steady_clock::time_point)> InputChangedHandler;

  Gpio(std::string gpioDevice = "/dev/gpiochip0");
  ~Gpio();

  // Setup the specified line with the specified mode and bias. 
  // May be called more than once to change settings
  // Must be called before calls to read, write, or subscribe.
  void setupLine(int line, LineMode mode, LineBias bias = LineBias::Off);

  // Release a line. After calling this, read, write, and subscribe amy no longer be called.
  void releaseLine(int line);

  // Get a line's active status as a bool, true if the line is active, false otherwise.
  bool read(int line);

  // Set a line active or inactive
  void write(int line, bool value);

  // Subscribe to change notifications when an inout signal changes
  sigslot::connection subscribe(int line, InputChangedHandler handler);

private:
  struct Line;
  std::map<int, std::unique_ptr<Line>> lines_;
  std::unique_ptr<std::thread> thread_;
  std::mutex mutex_;
  std::string gpioDevice_;
  bool stopThread_;
};