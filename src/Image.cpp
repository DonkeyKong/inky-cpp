#include "Image.hpp"
#include "Dither.hpp"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cmath>

// Hack to fix intellisense with some arm NEON code
#if __INTELLISENSE__
#undef __ARM_NEON
#undef __ARM_NEON__
#endif

#include <png.h>
#include <qrcodegen.hpp>
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

Image::Image(int width, int height, ImageFormat format )
{
    width_ = width;
    height_ = height;
    format_ = format;
    data_.resize(width_ * height_ * bytesPerPixel());
}

int Image::bytesPerPixel() const
{
  return format_ == ImageFormat::RGBA ? 4 : 1;
}

void Image::convert(ImageFormat format, ImageConversionSettings settings, Image* dest)
{
  if (dest == nullptr) dest = this;

  // Possible conversion types
  //                         from
  //               RGBA |  BW  |  BWR |  BWY
  //             +------+------+------+------+
  //        RGBA |  -   |  2   |   2  |   2  |
  //  to      BW |  1   |  -   |   3  |   3  |
  //         BWR |  1   |  -   |   -  |   -  |
  //         BWY |  1   |  -   |   -  |   -  |

  // This catches all the no-op ("-") conversions
  if (format == format_ || ((int)format > 1 && (int)format_ > 0))
  {
    // Image is the correct format already!
    // Just copy the data as-is
    if (dest != this)
    {
      dest->data_ = data_;
    }
  }
  // Conversion type 1: RGBA to BW/BWR/BWY
  else if (format_ == ImageFormat::RGBA)
  {
    Image inkyImage(width_, height_, format);

    if (settings.ditherMode == DitherMode::Pattern)
    {
      patternDither(*this, inkyImage);
    }
    else if (settings.ditherMode == DitherMode::Diffusion)
    {
      diffusionDither(*this, inkyImage, settings.ditherAccuracy);
    }

    // Move the new image buffer into place
    dest->data_ = std::move(inkyImage.data_);
  }
  // Conversion type 2: BW/BWR/BWY to RGBA
  else if (format == ImageFormat::RGBA)
  {
    RGBAColor rgbaLut[] = { {255, 255, 255, 255},   // White
                            {  0,   0,   0, 255},   // Black
                            {  0,   0,   0, 255} }; // Color
    if (format_ == ImageFormat::InkyBWR)
    {
      rgbaLut[2] = {255, 0, 0, 255};
    }
    else if (format_ == ImageFormat::InkyBWY)
    {
      rgbaLut[2] = {255, 255, 0, 255};
    }

    std::vector<uint8_t> dataRGBA(4 * width_ * height_);
    for (int i=0; i < data_.size(); ++i)
    {
      uint8_t c = data_[i];
      if (c < 3)
      {
          dataRGBA[i*4+0] = rgbaLut[c].R;
          dataRGBA[i*4+1] = rgbaLut[c].G;
          dataRGBA[i*4+2] = rgbaLut[c].B;
          dataRGBA[i*4+3] = rgbaLut[c].A;
      }
    }
    dest->data_ = std::move(dataRGBA);
  }
  // Conversion type 3: BWR/BWY to BW
  else if (format == ImageFormat::InkyBW)
  {
    if (dest != this)
    {
      dest->data_ = data_;
    }
    std::vector<uint8_t>& dataBW = dest->data_;
    InkyColor colorToBwSub = (format_ == ImageFormat::InkyBWR) ? InkyColor::Black : InkyColor::White;
    for (int i=0; i < dataBW.size(); ++i)
    {
      // Look for color accents and plaint them black (red) or white (yellow)
      if (dataBW[i] > (uint8_t)InkyColor::Black)
      {
        dataBW[i] = (uint8_t)colorToBwSub;
      }
    }
  }
  
  dest->width_ = width_;
  dest->height_ = height_;
  dest->format_ = format;
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
                           base::KernelTypeGaussian);
    dest->data_ = std::move(scaleData);
  }
  else 
  {
    std::vector<uint8_t> scaleData(uncroppedWidth * uncroppedHeight);
    base::ResampleImage<1>(data_.data(), (uint32_t)width_, (uint32_t)height_,
                           scaleData.data(), (uint32_t)uncroppedWidth, (uint32_t)uncroppedHeight,
                           base::KernelTypeLanczos3);
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

  // TBD: Fill in the background color

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


std::shared_ptr<Image> Image::FromPngFile(const std::string& imagePath)
{
    auto image = std::make_shared<Image>();
    image->readPng(imagePath);
    return image;
}

std::shared_ptr<Image> Image::FromQrPayload(const std::string& qrPayload)
{
    auto image = std::make_shared<Image>();
    
    auto qr = qrcodegen::QrCode::encodeText(qrPayload.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int quietZoneSize = 2;
    image->width_ = qr.getSize() + quietZoneSize * 2;
    image->height_ = qr.getSize() + quietZoneSize * 2;
    image->data_.resize(image->width_ * image->height_ * 4);
    auto dataPtr = image->data();

    for (int y = -quietZoneSize; y < qr.getSize() + quietZoneSize; y++)
    {
        for (int x = -quietZoneSize; x < qr.getSize() + quietZoneSize; x++)
        {
            bool pxIsDark = qr.getModule(x,y);
            dataPtr[0] = pxIsDark ? 0 : 255;
            dataPtr[1] = pxIsDark ? 0 : 255;
            dataPtr[2] = pxIsDark ? 0 : 255;
            dataPtr[3] = 255;
            dataPtr += 4;
        }
    }
    
    return image;
}

void Image::readPng(const std::string& filename)
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

void Image::writePng(const std::string& filename) 
{
  Image dest;
  Image* imgToSave = this;
  if (format_ != ImageFormat::RGBA)
  {
    imgToSave = &dest;
    convert(ImageFormat::RGBA, ImageConversionSettings(), imgToSave);
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
  uint8_t* data = imgToSave->data_.data();
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

ImageFormat Image::format() const
{
    return format_;
}

uint8_t& Image::operator[](std::size_t idx)
{
    return data_[idx];
}