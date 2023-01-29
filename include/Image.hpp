#pragma once

#include "Color.hpp"
#include "BoundingBox.hpp"
#include "base_resample.h"

#include <vector>
#include <string>
#include <memory>

enum class ImageFormat : int
{
  RGBA = 0,
  IndexedColor = 1
};

enum class ImageScaleMode
{
  Stretch,
  Fit,
  Fill
};

enum class ImageInterpolationMode
{
  Auto = -1,
  Nearest  = (int) base::KernelTypeNearest,
  Average  = (int) base::KernelTypeAverage,
  Bilinear = (int) base::KernelTypeBilinear,
  Bicubic  = (int) base::KernelTypeBicubic,
  Mitchell = (int) base::KernelTypeMitchell,
  Cardinal = (int) base::KernelTypeCardinal,
  BSpline  = (int) base::KernelTypeBSpline,
  Lanczos  = (int) base::KernelTypeLanczos,
  Lanczos2 = (int) base::KernelTypeLanczos2,
  Lanczos3 = (int) base::KernelTypeLanczos3,
  Lanczos4 = (int) base::KernelTypeLanczos4,
  Lanczos5 = (int) base::KernelTypeLanczos5,
  Catmull  = (int) base::KernelTypeCatmull,
  Gaussian = (int) base::KernelTypeGaussian,
};

enum class DitherMode
{
  Diffusion, // Uses Floyd–Steinberg dithering algo
  Pattern // Uses classic 17 pattern swatches
};

struct ImageDitherSettings
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

  ImageInterpolationMode interpolationMode = ImageInterpolationMode::Auto;

  // When scaling or cropping, this determines what color fills in the background if not all destination pixels are covered
  RGBAColor backgroundColor {255, 255, 255, 255};
};

class Image
{
public:
    // Construct an RGBA image with no data
    Image();

    // Construct an RGBA image with the specified size, allocating memory.
    Image(int width, int height);

    // Construct an indexed image with the specified size and color map, allocating memory.
    Image(int width, int height, IndexedColorMap colorMap);

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

    const IndexedColorMap& colorMap() const;

    // Get the bounding box of this image's pixel grid
    BoundingBox bounds() const;

    // Get the number of bytes per pixel
    int bytesPerPixel() const;

    // Convert an image to indexed format. Specify a destination image for the conversion
    // or leave dest as nullptr to perform an in-place conversion.
    void toIndexed(IndexedColorMap colorMap, ImageDitherSettings settings = ImageDitherSettings());
    void toIndexed(Image& dest, IndexedColorMap colorMap, ImageDitherSettings settings = ImageDitherSettings()) const;

    // Convert an image to RGBA format
    void toRGBA();
    void toRGBA(Image& dest) const;

    // Scale the image to the specified size. Specify a destination image for the operation
    // or leave dest as nullptr to perform the operation in-place.
    void scale(int width, int height, ImageScaleSettings settings = ImageScaleSettings(), Image* dest = nullptr);

    // Crop the image to the specified size rectangle. 
    // "Out of bounds" x, y, width, and height are ok, and will infill with a background color.
    // Specify a destination image for the operation or leave dest as nullptr to perform the operation in-place.
    void crop(int x, int y, int width, int height, ImageScaleSettings settings = ImageScaleSettings(), Image* dest = nullptr);

    // Read a PNG image from disk. Will change format to RGBA and reset dimensions.
    void readPngFromFile(const std::string& imagePath);

    // Write a PNG image to disk.
    void writePngToFile(const std::string& imagePath) const;

    // Read a JPG image from a file. Will change format to RGBA and reset dimensions.
    void readJpegFromFile(const std::string& filename);

    // Read a JPG image from buffer. Will change format to RGBA and reset dimensions.
    void readJpegFromBuffer(const std::vector<uint8_t>& buffer);
    void readJpegFromBuffer(const std::string& buffer);
    void readJpegFromBuffer(const uint8_t* buffer, size_t len);

    // Write a JPG image to disk.
    void writeJpegToFile(const std::string& filename, int quality = 75) const;

    // Write a JPG image to a memory buffer
    std::vector<uint8_t> writeJpegToBuffer(int quality = 75) const;

    // Open a file from disk and construct an image around it
    static Image FromFile(const std::string& imagePath);

    // Open a file from disk and construct an image around it
    static Image FromBuffer(const std::vector<uint8_t>& imageBuf);
    static Image FromBuffer(const std::string& buffer);
    static Image FromBuffer(const uint8_t* buffer, size_t len);
private:
    int width_, height_;
    ImageFormat format_;
    std::vector<uint8_t> data_;
    IndexedColorMap colorMap_;
};
