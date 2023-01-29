#include "Image.hpp"
#include "Dither.hpp"

#include <fmt/format.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cmath>
#include <fstream>
#include <filesystem>

// Hack to fix intellisense with some arm NEON code
#if __INTELLISENSE__
#undef __ARM_NEON
#undef __ARM_NEON__
#endif

#include <png.h>
#include <turbojpeg.h>
#include <base_resample.h>

static void abort_(const char *s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

Image::Image()
{
    width_ = 0;
    height_ = 0;
    format_ = ImageFormat::RGBA;
}

Image::Image(int width, int height)
{
    width_ = width;
    height_ = height;
    format_ = ImageFormat::RGBA;
    data_.resize(width_ * height_ * bytesPerPixel());
}

Image::Image(int width, int height, IndexedColorMap colorMap)
{
    width_ = width;
    height_ = height;
    format_ = ImageFormat::IndexedColor;
    colorMap_ = colorMap;
    data_.resize(width_ * height_ * bytesPerPixel());
}

int Image::bytesPerPixel() const
{
  return format_ == ImageFormat::RGBA ? 4 : 1;
}

void Image::toIndexed(IndexedColorMap colorMap, ImageDitherSettings settings)
{
  toIndexed(*this, colorMap, settings);
}

void Image::toIndexed(Image& dest, IndexedColorMap colorMap, ImageDitherSettings settings) const
{
  // Conversion type 1: RGBA to indexed
  if (format_ == ImageFormat::RGBA)
  {
    Image indexImage(width_, height_, colorMap);

    if (settings.ditherMode == DitherMode::Pattern)
    {
      patternDither(*this, indexImage);
    }
    else if (settings.ditherMode == DitherMode::Diffusion)
    {
      diffusionDither(*this, indexImage, settings.ditherAccuracy);
    }

    // Move the new image buffer into place
    dest.data_ = std::move(indexImage.data_);
    dest.width_ = width_;
    dest.height_ = height_;
    dest.format_ = ImageFormat::IndexedColor;
    dest.colorMap_ = colorMap;
    return;
  }
  // Conversion type 2: indexed to indexed (via RGBA)
  else
  {
    Image rgbaImage(width_, height_);
    toRGBA(rgbaImage);
    rgbaImage.toIndexed(dest, colorMap, settings);
    return;
  }
}

void Image::toRGBA()
{
  toRGBA(*this);
}

void Image::toRGBA(Image& dest) const
{
  bool inPlace = (&dest == this);

  // Image is the correct format already!
  // Just copy the data as-is
  if (format_ == ImageFormat::RGBA)
  {
    if (!inPlace)
    {
      dest.data_ = data_;
    }
  }
  // Conversion type 2: BW/BWR/BWY to RGBA
  else
  {
    std::vector<uint8_t> dataRGBA(4 * width_ * height_);
    RGBAColor* rgbaPtr = (RGBAColor*)dataRGBA.data();
    IndexedColor* indexedPtr = (IndexedColor*) data_.data();
    for (int i=0; i < data_.size(); ++i)
    {
      rgbaPtr[i] = colorMap_.toRGBAColor(indexedPtr[i]);
    }
    dest.data_ = std::move(dataRGBA);
  }
  
  // Set the size and format on the destination image
  dest.width_ = width_;
  dest.height_ = height_;
  dest.format_ = ImageFormat::RGBA;
  return;
}

void Image::scale(int width, int height, ImageScaleSettings settings, Image* dest)
{
  if (dest == nullptr) dest = this;

  float xScale = (float)width / (float)width_;
  float yScale = (float)height / (float)height_;

  int uncroppedWidth = width;
  int uncroppedHeight = height;

  if (settings.scaleMode == ImageScaleMode::Fill)
  {
    float scale = std::max(xScale, yScale);
    uncroppedWidth = (int)((float)width_ * scale);
    uncroppedHeight = (int)((float)height_ * scale);
  }
  else if (settings.scaleMode == ImageScaleMode::Fit)
  {
    float scale = std::min(xScale, yScale);
    uncroppedWidth = (int)((float)width_ * scale);
    uncroppedHeight = (int)((float)height_ * scale);
  }

  if (settings.interpolationMode == ImageInterpolationMode::Auto)
  {
    // Use Bilinear for enlargement and Gaussian for reduction
    settings.interpolationMode = (width > width_) ? ImageInterpolationMode::Bilinear : 
                                                    ImageInterpolationMode::Gaussian;
  }

  if (width == width_ && height == height_)
  {
    // Image is the correct size already!
    // Just copy the data as-is
    if (dest != this)
    {
      dest->data_ = data_;
    }
  }
  else if (format_ == ImageFormat::RGBA)
  {
    std::vector<uint8_t> scaleData(uncroppedWidth * uncroppedHeight * 4);
    base::ResampleImage<4>(data_.data(), (uint32_t)width_, (uint32_t)height_,
                           scaleData.data(), (uint32_t)uncroppedWidth, (uint32_t)uncroppedHeight,
                           (base::KernelType)settings.interpolationMode);
    dest->data_ = std::move(scaleData);
  }
  else 
  {
    std::vector<uint8_t> scaleData(uncroppedWidth * uncroppedHeight);
    base::ResampleImage<1>(data_.data(), (uint32_t)width_, (uint32_t)height_,
                           scaleData.data(), (uint32_t)uncroppedWidth, (uint32_t)uncroppedHeight,
                           (base::KernelType)settings.interpolationMode);
    dest->data_ = std::move(scaleData);
  }
  
  dest->width_ = uncroppedWidth;
  dest->height_ = uncroppedHeight;
  dest->format_ = format_;

  if (width != uncroppedWidth || height != uncroppedHeight)
  {
    dest->crop((uncroppedWidth - width)/2, (uncroppedHeight - height)/2, width, height, settings);
  }
}

void Image::crop(int x, int y, int width, int height, ImageScaleSettings settings, Image* dest)
{
  if (dest == nullptr) dest = this;
  if (x == 0 && y == 0 && width == width_ && height_ == height)
  {
    if (dest != this)
    {
      dest->width_ = width_;
      dest->height_ = height_;
      dest->format_ = format_;
      dest->data_ = data_;
    }
    return;
  }

  // Create a buffer for the cropped image
  std::vector<uint8_t> croppedData(width*height*bytesPerPixel());
  int size = width * height;

  // Fill in the background color
  if (format_ == ImageFormat::RGBA)
  {
    RGBAColor* data = (RGBAColor*)croppedData.data();
    for (int i=0; i < size; ++i)
    {
      data[i] = settings.backgroundColor;
    }
  }
  else
  {
    IndexedColor backgroundColor = colorMap_.toIndexedColor(settings.backgroundColor);
    IndexedColor* data = (IndexedColor*)croppedData.data();
    for (int i=0; i < size; ++i)
    {
      data[i] = backgroundColor;
    }
  }

  // Figure out the copy boundaries
  int srcX = (x < 0) ? 0 : x;
  int srcY = (y < 0) ? 0 : y;
  int dstX = (x > 0) ? 0 : -x;
  int dstY = (y > 0) ? 0 : -y;
  int cpyWidth = std::min(width-dstX,width_-srcX);
  int cpyHeight = std::min(height-dstY,height_-srcY);

  // Do the copy row by row
  for (int row = 0; row < cpyHeight; ++row)
  {
    uint8_t* src = data_.data() + (srcX+(srcY+row)*width_) * bytesPerPixel();
    uint8_t* dst = croppedData.data() + (dstX+(dstY+row)*width) * bytesPerPixel();
    memcpy(dst, src, cpyWidth*bytesPerPixel());
  }

  // Move the cropped data into place
  dest->width_ = width;
  dest->height_ = height;
  dest->format_ = format_;
  dest->data_ = std::move(croppedData);
}

const IndexedColorMap& Image::colorMap() const
{
  return colorMap_;
}

Image Image::FromFile(const std::string& imagePath)
{
    Image image;
    std::string ext = std::filesystem::path(imagePath).extension();
    std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );

    if (ext == ".png")
    {
      image.readPngFromFile(imagePath);
    }
    else if (ext == ".jpg" || ext == ".jpeg" || ext == ".jpe")
    {
      image.readJpegFromFile(imagePath);
    }
    else
    {
      throw std::runtime_error(fmt::format("Unsupported extension '{}'! Only png and jpeg are supported.", ext));
    }

    return image;
}

Image Image::FromBuffer(const std::string& imageBuf)
{
  return FromBuffer((uint8_t*)imageBuf.data(), imageBuf.size());
}

Image Image::FromBuffer(const std::vector<uint8_t>& imageBuf)
{
  return FromBuffer(imageBuf.data(), imageBuf.size());
}

Image Image::FromBuffer(const uint8_t* imageBuf, size_t len)
{
  Image image;

  if (len < 16)
  {
    throw std::runtime_error("Buffer is too short to contain an image.");
  }

  if (imageBuf[0] == 0xFF && imageBuf[1] == 0xD8)
  {
    image.readJpegFromBuffer(imageBuf, len);
  }
  else
  {
    throw std::runtime_error("Unsupported image buffer format! Only jpeg is supported at this time.");
  }

  return image;
}

void Image::readJpegFromFile(const std::string& filename)
{
  std::basic_ifstream<uint8_t> file(filename, std::ios::binary);
  std::vector<uint8_t> compressedImage((std::istreambuf_iterator<uint8_t>(file)), std::istreambuf_iterator<uint8_t>());
  readJpegFromBuffer(compressedImage);
}

void Image::readJpegFromBuffer(const std::vector<uint8_t>& compressedImage)
{
  readJpegFromBuffer(compressedImage.data(), compressedImage.size());
}

void Image::readJpegFromBuffer(const std::string& compressedImage)
{
  readJpegFromBuffer((uint8_t*)compressedImage.data(), compressedImage.size());
}

void Image::readJpegFromBuffer(const uint8_t* compressedImage, size_t len)
{
  int jpegSubsamp;
  tjhandle _jpegDecompressor = tjInitDecompress();
  // const cast to deal with C api
  tjDecompressHeader2(_jpegDecompressor, const_cast<uint8_t*>(compressedImage), len, &width_, &height_, &jpegSubsamp);
  format_ = ImageFormat::RGBA;
  data_.resize(width_*height_*4);
  tjDecompress2(_jpegDecompressor, compressedImage, len, data_.data(), width_, 0/*pitch*/, height_, TJPF_RGBA, TJFLAG_FASTDCT);
  tjDestroy(_jpegDecompressor);
}

void Image::writeJpegToFile(const std::string& filename, int quality) const
{
  if (width_ < 1 || height_ < 1)
  {
    throw std::runtime_error("Cannot save zero-dimension image!");
  }

  Image dest;
  const Image* imgToSave = this;
  if (format_ != ImageFormat::RGBA)
  {
    toRGBA(dest);
    imgToSave = &dest;
  }

  long unsigned int jpegSize = 0;
  uint8_t* compressedImage = NULL; //!< Memory is allocated by tjCompress2 if _jpegSize == 0

  tjhandle jpegCompressor = tjInitCompress();

  tjCompress2(jpegCompressor, imgToSave->data(), imgToSave->width_, 0, imgToSave->height_, TJPF_RGBA,
              &compressedImage, &jpegSize, TJSAMP_444, quality,
              TJFLAG_FASTDCT);

  tjDestroy(jpegCompressor);

  // Save the buffer to a file
  try
  {
    std::basic_ofstream<uint8_t> file(filename, std::ios::binary | std::ios::trunc);
    file.write(compressedImage, jpegSize);
    file.close();
  }
  catch (...)
  {
    tjFree(compressedImage);
    std::rethrow_exception(std::current_exception());
  }
  tjFree(compressedImage);
}

std::vector<uint8_t> Image::writeJpegToBuffer(int quality) const
{
  if (width_ < 1 || height_ < 1)
  {
    throw std::runtime_error("Cannot save zero-dimension image!");
  }

  Image dest;
  const Image* imgToSave = this;
  if (format_ != ImageFormat::RGBA)
  {
    toRGBA(dest);
    imgToSave = &dest;
  }

  long unsigned int jpegSize = 0;
  uint8_t* compressedImage = NULL; //!< Memory is allocated by tjCompress2 if _jpegSize == 0

  tjhandle jpegCompressor = tjInitCompress();

  tjCompress2(jpegCompressor, imgToSave->data(), imgToSave->width_, 0, imgToSave->height_, TJPF_RGBA,
              &compressedImage, &jpegSize, TJSAMP_444, quality,
              TJFLAG_FASTDCT);

  tjDestroy(jpegCompressor);

  std::vector<uint8_t> imgBuf(compressedImage, compressedImage+jpegSize);
  tjFree(compressedImage);
  return imgBuf;
}

void Image::readPngFromFile(const std::string& filename)
{
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep* row_pointers;

    char header[8]; // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp)
        abort_("[read_png_file] File %s could not be opened for reading", filename.c_str());
    fread(header, 1, 8, fp);
    if (png_sig_cmp((png_bytep)header, 0, 8))
        abort_("[read_png_file] File %s is not recognized as a PNG file", filename.c_str());

    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width_ = png_get_image_width(png_ptr, info_ptr);
    height_ = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during read_image");

    // Size dest appropriately
    int strideInBytes = png_get_rowbytes(png_ptr, info_ptr);
    data_.resize(height_ * strideInBytes);

    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height_);
    for (int y = 0; y < height_; y++)
    {
        row_pointers[y] = data_.data() + strideInBytes * y;
    }

    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    // It is possible that the image was read in as RGB instead of RGBA. Let's fix that here.
    if (strideInBytes == width_ * 3)
    {
        std::vector<uint8_t> rgbaVec(height_ * width_ * 4);
        uint8_t* rgbData = data_.data();
        uint8_t* rgbaData = rgbaVec.data();
        int pixels = width_ * height_;
        for (int p=0; p < pixels; p++)
        {
            rgbaData[0] = rgbData[0];
            rgbaData[1] = rgbData[1];
            rgbaData[2] = rgbData[2];
            rgbaData[3] = 255;
            rgbaData += 4;
            rgbData += 3;
        }
        data_ = std::move(rgbaVec);
    }
}

void Image::writePngToFile(const std::string& filename) const
{
  if (width_ < 1 || height_ < 1)
  {
    throw std::runtime_error("Cannot save zero-dimension image!");
  }

  Image dest;
  const Image* imgToSave = this;
  if (format_ != ImageFormat::RGBA)
  {
    toRGBA(dest);
    imgToSave = &dest;
  }

  int y;

  FILE *fp = fopen(filename.c_str(), "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width_, height_,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  std::vector<uint8_t*> rows(height_);
  uint8_t* data = const_cast<uint8_t*>(imgToSave->data()); // const cast to deal with C api
  for (int y=0; y < height_; ++y)
  {
    rows[y] = data+(y*width_*4);
  }

  png_write_image(png, rows.data());
  png_write_end(png, NULL);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}

uint8_t* Image::data()
{
    return data_.data();
}

const uint8_t* Image::data() const
{
    return data_.data();
}

int Image::width() const
{
    return width_;
}

int Image::height() const
{
    return height_;
}

BoundingBox Image::bounds() const
{
  return {0, 0, width_, height_};
}

ImageFormat Image::format() const
{
    return format_;
}