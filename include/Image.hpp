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
  InkyBWY = 3,
  SevenColor = 5
};

enum class ImageScaleMode
{
  Stretch,
  Fit,
  Fill
};

enum class DitherMode
{
  Diffusion, // Uses Floyd–Steinberg dithering algo
  Pattern // Uses classic 17 pattern swatches
};

struct ImageConversionSettings
{
  // When converting from RGBA to an Inky image mode, this dithering mode will be used.
  DitherMode ditherMode = DitherMode::Diffusion;

  // Lower accuracy allows a cleaner result
  // Sane values: (0.5 - 1.0)
  float ditherAccuracy = 1.0f;
};

struct ImageScaleSettings
{
  // Determines aspect and scale of image resizing
  // "Stretch" changes the source image aspect ratio to match the destination image
  // "Fill" maintains the source image aspect ratio and scales it large enough to fill all destination pixels
  // "Fit" maintains the source image aspect ratio and scales it to fit inside the destination image
  ImageScaleMode scaleMode = ImageScaleMode::Stretch;

  // When scaling or cropping, this determines what color fills in the background if not all destination pixels are covered
  RGBAColor backgroundColor;
};

class Image
{
public:
    // Construct a 0x0 image with no data allocated
    Image();

    // Construct an image with the specified size and format, allocating memory.
    Image(int width, int height, ImageFormat = ImageFormat::RGBA);

    // Get a pointer to the first pixel element
    uint8_t* data();

    // Get a const pointer to the first pixel element
    const uint8_t* data() const;

    // Get the image width
    int width() const;

    // Get the image height
    int height() const;

    // Get the image format
    ImageFormat format() const;

    // Get the number of bytes per pixel
    int bytesPerPixel() const;

    // Convert the image to Inky or RGBA formats. Specify a destination image for the conversion
    // or leave dest as nullptr to perform an in-place conversion.
    void convert(ImageFormat format, ImageConversionSettings settings = ImageConversionSettings(), Image* dest = nullptr);

    // Scale the image to the specified size. Specify a destination image for the operation
    // or leave dest as nullptr to perform the operation in-place.
    void scale(int width, int height, ImageScaleSettings settings = ImageScaleSettings(), Image* dest = nullptr);

    // Crop the image to the specified size rectangle. 
    // "Out of bounds" x, y, width, and height are ok, and will infill with a background color.
    // Specify a destination image for the operation or leave dest as nullptr to perform the operation in-place.
    void crop(int x, int y, int width, int height, ImageScaleSettings settings = ImageScaleSettings(), Image* dest = nullptr);

    // Read a PNG image from disk. Will change format to RGBA and reset dimensions.
    void readPng(const std::string& imagePath);

    // Write a PNG image to disk. Will change format to RGBA on write.
    void writePng(const std::string& imagePath);

    // Open a PNG file from disk and construct an image around it
    static std::shared_ptr<Image> FromPngFile(const std::string& imagePath);

    // Create a new image containing a QR code
    static std::shared_ptr<Image> FromQrPayload(const std::string& qrPayload);

    // Access pixel data directly from the image
    uint8_t& operator[](std::size_t idx);
private:
    int width_, height_;
    ImageFormat format_;
    std::vector<uint8_t> data_;
};
