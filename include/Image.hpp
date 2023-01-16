#pragma once

#include "Color.hpp"

#include <vector>
#include <string>
#include <memory>

enum class ImageFormat : int
{
  RGBA = 0,
  InkyBW = 1,
  InkyBWR = 2,
  InkyBWY = 3
};

enum class ImageScaleMode
{
  Stretch,
  Fit,
  Fill
};

enum class ImageConvertMode
{
  Threshold,
  //Dither,
  //SomeSuperRadShitIllNeverImplement
};

class Image
{
public:
    Image();
    Image(int width, int height, ImageFormat = ImageFormat::RGBA);
    uint8_t* data();
    const uint8_t* data() const;
    int width() const;
    int height() const;
    ImageFormat format() const;
    int bytesPerPixel() const;
    // Convert the image to Inky or RGBA formats. Specify a destination image for the conversion
    // or leave dest as nullptr to perform an in-place conversion.
    void convert(ImageFormat format, ImageConvertMode mode = ImageConvertMode::Threshold, Image* dest = nullptr);
    // Scale the image to the specified size. Specify a destination image for the operation
    // or leave dest as nullptr to perform the operation in-place.
    void scale(int width, int height, ImageScaleMode mode = ImageScaleMode::Stretch, Image* dest = nullptr);
    void crop(int x, int y, int width, int height, Image* dest = nullptr);
    void readPng(const std::string& imagePath);
    void writePng(const std::string& imagePath);
    static std::shared_ptr<Image> FromPngFile(const std::string& imagePath);
    static std::shared_ptr<Image> FromQrPayload(const std::string& qrPayload);
    uint8_t& operator[](std::size_t idx);
private:
    //RGBAColor sampleRGBA(float x, float y);
    //InkyColor sampleInky(float x, float y);
    int width_, height_;
    ImageFormat format_;
    std::vector<uint8_t> data_;
};
