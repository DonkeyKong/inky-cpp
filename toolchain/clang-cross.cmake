# This toolchain file allows an Apple Silicon Mac with clang and
# a Raspberry Pi sysroot in its home directory to painlessly build 
# RPi binaries under macOS
# Download the RPi Sysroot here: [No good link exists yet]

# I created this sysroot by mounting the rootfs of
# a RPi OS image, going to raspberry pi root, then
# running the following command:
#      sudo rsync -rzLR --safe-links \
#            usr/lib/arm-linux-gnueabihf \
#            usr/lib/gcc/arm-linux-gnueabihf \
#            usr/include \
#            lib/arm-linux-gnueabihf \
#            /opt/raspios-bullseye-armhf-lite-sysroot/ 

# the name of the target operating system
set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_PROCESSOR "armv7")

# which compilers to use for C and C++
set(triple arm-linux-gnueabihf)
set(CMAKE_C_COMPILER   "clang")
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# Allows cross-compilation
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# where is the target environment located
set(CMAKE_SYSROOT "/opt/raspios-bullseye-armhf-lite-sysroot")
set(CMAKE_FIND_ROOT_PATH  "/opt/raspios-bullseye-armhf-lite-sysroot")

# Fix a bug with locating zlib
set(ZLIB_LIBRARY "${CMAKE_SYSROOT}/lib/arm-linux-gnueabihf/libz.a")

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
