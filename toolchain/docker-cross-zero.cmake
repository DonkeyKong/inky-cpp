# This toolchain file allows a debian slim docker to painlessly
# build RPi Zero binaries.
# The x-tools chain comes from here: 
# https://github.com/tttapa/docker-arm-cross-toolchain/releases/download/0.0.9/x-tools-armv6-rpi-linux-gnueabihf.tar.xz

# the name of the target operating system
set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_PROCESSOR "armv6")

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   "/mnt/toolchain/x-tools/armv6-rpi-linux-gnueabihf/bin/armv6-rpi-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/mnt/toolchain/x-tools/armv6-rpi-linux-gnueabihf/bin/armv6-rpi-linux-gnueabihf-g++")

# Allows cross-compilation
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# where is the target environment located
set(CMAKE_SYSROOT "/mnt/sysroot")
set(CMAKE_FIND_ROOT_PATH  "/mnt/sysroot")

set(CMAKE_C_FLAGS "-march=armv6 -mfpu=vfp -mfloat-abi=hard" CACHE STRING "Flags for Raspberry PI 1 B+ Zero" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags for Raspberry PI 1 B+ Zero" FORCE)

# Fix a bug with locating zlib
set(ZLIB_LIBRARY "${CMAKE_SYSROOT}/lib/arm-linux-gnueabihf/libz.a")

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
