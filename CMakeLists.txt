cmake_minimum_required(VERSION 3.16)

project("inky-cpp")

set(CMAKE_CXX_STANDARD 17)

find_file(BCM_HOST_PATH NAMES "bcm_host.h")
message("BCM host header: ${BCM_HOST_PATH}")

add_executable( ${PROJECT_NAME} 
                    src/Color.cpp
                    src/Image.cpp
                    src/Dither.cpp
                    src/Inky.cpp
                    src/I2CDevice.cpp
                    src/SPIDevice.cpp
                    src/HttpService.cpp
                    deps/QR-Code-generator/cpp/qrcodegen.cpp
                    src/main.cpp )

# openssl
if (APPLE)
    set (OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
endif()
find_package(OpenSSL REQUIRED)

# httplib - Cmake, header-only
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE deps/cpp-httplib)

# fmt - Cmake, header-only
target_compile_definitions(${PROJECT_NAME} PUBLIC "FMT_HEADER_ONLY")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE deps/fmt/include)

# json - Cmake, header-only
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE deps/json/include)

# libpng, we build this ourselves mostly for fun
if (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
set(PNG_ARM_NEON on CACHE bool "" FORCE)
message("Apple Silicon detected, manually forcing ARM NEON optimizations for libpng")
endif()
set(PNG_SHARED OFF)
set(PNG_TESTS OFF)
add_subdirectory("deps/libpng")
set(PNG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/deps/libpng
                    ${CMAKE_CURRENT_BINARY_DIR}/deps/libpng)

# libjpeg-turbo, we build this ourselves mostly for fun
include(ExternalProject)
set(LIBJPEG_TURBO_INSTALL_PATH "${CMAKE_BINARY_DIR}/libjpeg-turbo_install")
ExternalProject_Add(libjpeg-turbo
  SOURCE_DIR        "${PROJECT_SOURCE_DIR}/deps/libjpeg-turbo"
  BINARY_DIR        "${CMAKE_BINARY_DIR}/libjpeg-turbo_build"
  INSTALL_DIR       "${LIBJPEG_TURBO_INSTALL_PATH}"
  CMAKE_ARGS        "-DENABLE_SHARED:BOOL=FALSE"
                    "-DENABLE_STATIC:BOOL=TRUE"
                    "-DCMAKE_INSTALL_PREFIX:PATH=${LIBJPEG_TURBO_INSTALL_PATH}"
                    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
)
add_library(libjpeg INTERFACE)
target_include_directories(libjpeg INTERFACE ${LIBJPEG_TURBO_INSTALL_PATH}/include)
target_link_libraries(libjpeg INTERFACE ${LIBJPEG_TURBO_INSTALL_PATH}/lib/libturbojpeg.a)

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

add_dependencies(${PROJECT_NAME} libjpeg-turbo)

target_include_directories(${PROJECT_NAME} PRIVATE 
                              include 
                              ${PNG_INCLUDE_DIR} 
                              deps/QR-Code-generator/cpp 
                              deps/eigen
                              deps/single-header-image-resampler
                              deps/magic_enum/include )
target_link_libraries(${PROJECT_NAME} png_static libjpeg)
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)
if (BCM_HOST_PATH)
  target_link_libraries(${PROJECT_NAME} stdc++fs bcm_host pthread)
endif()

add_custom_target(copy_resources ALL)

add_custom_command(TARGET copy_resources POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/resources
                    ${CMAKE_CURRENT_BINARY_DIR}/resources)