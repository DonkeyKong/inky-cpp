# This toolchain file allows a debian slim docker to painlessly
# build RPi binaries.
# You'll need the following packages installed:
#   arm-linux-gnueabihf-gcc
#   g++-arm-linux-gnueabihf
#   cmake
# You'll also need a Raspberry Pi sysroot at /mnt/sysroot 
# Check out get_sysroot.sh to see how you might get that
# going.

# Select your minimum Raspberry Pi version
set(RASPBERRY_VERSION 1)  # 1 = Raspberry PI 1 B+ Zero
                          # 2 = Raspberry PI 2
                          # 3 = Raspberry PI 3

# the name of the target operating system
set(CMAKE_SYSTEM_NAME "Linux")
if(RASPBERRY_VERSION VERSION_GREATER 1)
	set(CMAKE_SYSTEM_PROCESSOR "armv7")
else()
	set(CMAKE_SYSTEM_PROCESSOR "arm")
endif()

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   "/usr/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/arm-linux-gnueabihf-g++")

# Allows cross-compilation
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# where is the target environment located
set(CMAKE_SYSROOT "/mnt/sysroot")
set(CMAKE_FIND_ROOT_PATH  "/mnt/sysroot")

if(RASPBERRY_VERSION VERSION_GREATER 2)
	set(CMAKE_C_FLAGS "-mcpu=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard" CACHE STRING "Flags for Raspberry PI 3" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags for Raspberry PI 3" FORCE)
elseif(RASPBERRY_VERSION VERSION_GREATER 1)
	set(CMAKE_C_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard" CACHE STRING "Flags for Raspberry PI 2" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags for Raspberry PI 2" FORCE)
else()
	set(CMAKE_C_FLAGS "-march=armv6 -mfpu=vfp -mfloat-abi=hard" CACHE STRING "Flags for Raspberry PI 1 B+ Zero" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags for Raspberry PI 1 B+ Zero" FORCE)
endif()

# Fix a bug with locating zlib
set(ZLIB_LIBRARY "${CMAKE_SYSROOT}/lib/arm-linux-gnueabihf/libz.a")

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
