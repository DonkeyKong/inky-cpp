cmake_minimum_required(VERSION 3.16)

project("inky-cpp")

set(CMAKE_CXX_STANDARD 17)

find_file(BCM_HOST_PATH NAMES "bcm_host.h")
message("BCM host header: ${BCM_HOST_PATH}")

# libpng, we build this ourselves because oh my god operating systems
if (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
set(PNG_ARM_NEON on CACHE bool "" FORCE)
message("Apple Silicon detected, manually forcing ARM NEON optimizations for libpng")
endif()
set(PNG_SHARED OFF)
set(PNG_TESTS OFF)
add_subdirectory("deps/libpng")
set(PNG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/deps/libpng
                    ${CMAKE_CURRENT_BINARY_DIR}/deps/libpng)

add_executable( ${PROJECT_NAME} 
                    src/Color.cpp
                    src/Image.cpp
                    src/Dither.cpp
                    src/Inky.cpp
                    src/I2CDevice.cpp
                    deps/QR-Code-generator/cpp/qrcodegen.cpp
                    src/main.cpp )

# Set some compiler flags used for features
if (BCM_HOST_PATH)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "PI_HOST")       
    target_link_libraries(${PROJECT_NAME} bcm_host)
else()

endif()

# Link the c++ filesystem API under GCC
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
                              include 
                              ${PNG_INCLUDE_DIR} 
                              deps/QR-Code-generator/cpp 
                              deps/eigen
                              deps/single-header-image-resampler
                              deps/magic_enum/include )
target_link_libraries(${PROJECT_NAME} png_static)
