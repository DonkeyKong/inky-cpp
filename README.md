# inky-cpp
inky-cpp is a C++ rewrite of [Pimoroni's Inky display library](https://github.com/pimoroni/inky). This is not a fork or a port, so it will not be kept in sync automatically or have perfect feature parity.

This project includes both the inky-cpp library and a test application that allows a Raspberry Pi + Inky to act as network-updatable photo frame.

## Displays Supported
- [Inky Impression 5.7"](https://shop.pimoroni.com/products/inky-impression-5-7) (UC8159 driver chip)
- [Inky Impression 4"](https://shop.pimoroni.com/products/inky-impression-4) (UC8159 driver chip)
- [Inky wHAT](https://shop.pimoroni.com/products/inky-what) (SSD1683 driver chip)

Other Inky displays should be relatively simple to add, as the code is written to be easily expanded, but without older hardware to test on, or any reason to put in this work, I have not.

## Hardware Setup
1. Plug the Inky hat into the Raspberry Pi
2. Enable I2C and SPI using raspi-config
   * `sudo raspi-config`
   * Select **Interface Options**
   * Select **SPI** then **Yes**
   * Select **I2C** then **Yes**
   * Exit `raspi-config`
3. `sudo reboot`

## Building inky-cpp
The easiest but slowest way to build inky-cpp is on the Raspberry Pi itself. My starting point is a fresh install **Raspberry Pi OS Lite (32-bit)** (Bullseye) with networking and SSH setup. The steps should be similar for **64-bit** or **full** distros. The Pi will need access to the internet to fetch tools and sources.

> **Extra Credit**: I leave it as an exercise to the reader to set up a functioning RPi cross-compilation environment 😎

### Setup Environment
Running the following commands on a fresh Raspberry Pi will ensure you get all the tools needed to clone and build the project.

* `sudo apt-get update`
* `sudo apt-get install git cmake`

### Clone the repository with all submodules
Using git, fetch all the inky-cpp sources. You can do this in any directory you like. The home directory is fine.

> `git clone --recurse-submodules https://github.com/DonkeyKong/inky-cpp.git`

### Build
With all the sources downloaded, it's time to build. Run the following commands to build inky-cpp:
* `cd inky-cpp`
* `mkdir build`
* `cd build`
* `cmake -DCMAKE_BUILD_TYPE=Release ..`
* `make all`

> **Note**: Replace `Release` in the cmake command for e.g.: `Debug` to build a different configuration. These instructions are written for a novice, feel free to deviate if you know what you're doing.

### Run
With everything built, all that's left is to run the test app. Invoke it with

* `./inky-cpp`

You'll see some diagnostic text print out and if all is well, the Inky display will refresh to show a QR code. Scan the code with a phone or visit the listed URL to access the web UI. Play with the test app via the web UI. When you are done, exit by pressing `ctrl-c`

You are now ready to consume `libinky.a` in your own C++ code, or you can setup the Raspberry Pi to run the test application on boot.

## Running the Test App on Boot
> **To Do**: Write a section on using systemd to autolaunch inky test app...

## Submodule Dependencies
This repositiory pulls in several submodules that inky-cpp and its test application depend on. These submodules are configured to build automatically, so there is nothing that you need to do. This section is informational only, to explain what each of these are and what they do.

### libpng
Built into the image class to allow creating image objects from png files or streams.

### libjpeg-turbo
Built into the image class to allow creating image objects from jpeg files or streams.

### single-header-image-resampler
Built into the image class. Header-only lib takes care of creating high-quality upscales and downscales of images.

### fmt
Header-only library for formatting C++ strings. Not strictly necessary but sprintf and stream formatting is worse.

### magic_enum *(Test app only)*
Allows painless pretty printing enum values. Used by the test app strictly for diagnostic output

### QR-Code-generator *(Test app only)*
A nice small library for generating bitmap QR codes. Used by the test app to make the QR code image that's displayed for connecting to the web UI

### cpp-httplib *(Test app only)*
Lightweight, header-only library provides HTTP services for the test app.

### json *(Test app only)*
Header-only library for parsing and forming json. Used by the test app for interacting with the web interface.

## Known Issues
Too many to enumerate at this point.