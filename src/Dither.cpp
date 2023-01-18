#include "Dither.hpp"
#include <stdexcept>

static const uint8_t ditherLut[] 
{
  // 0x00
  0, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  // 0x10
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  1, 0, 0, 0,
  // 0x20
  0, 0, 0, 0, 
  0, 0, 1, 0,
  0, 0, 0, 0,
  1, 0, 0, 0,
  // 0x30
  0, 0, 0, 0, 
  1, 0, 1, 0,
  0, 0, 0, 0,
  1, 0, 0, 0,
  // 0x40
  0, 0, 0, 0,
  1, 0, 1, 0,
  0, 0, 0, 0,
  1, 0, 1, 0,
  // 0x50
  0, 0, 0, 0,
  1, 0, 1, 0,
  0, 1, 0, 0,
  1, 0, 1, 0,
  // 0x60
  0, 0, 0, 1,
  1, 0, 1, 0,
  0, 1, 0, 0,
  1, 0, 1, 0,
  // 0x70
  0, 0, 0, 1,
  1, 0, 1, 0,
  0, 1, 0, 1,
  1, 0, 1, 0,
  // 0x80
  0, 1, 0, 1,
  1, 0, 1, 0,
  0, 1, 0, 1,
  1, 0, 1, 0,
  // 0x90
  0, 1, 0, 1,
  1, 0, 1, 0,
  0, 1, 0, 1,
  1, 1, 1, 0,
  // 0xA0
  0, 1, 0, 1,
  1, 0, 1, 1,
  0, 1, 0, 1,
  1, 1, 1, 0,
  // 0xB0
  0, 1, 0, 1,
  1, 0, 1, 1,
  0, 1, 0, 1,
  1, 1, 1, 1,
  // 0xC0
  0, 1, 0, 1,
  1, 1, 1, 1,
  0, 1, 0, 1,
  1, 1, 1, 1,
  // 0xD0
  0, 1, 0, 1,
  1, 1, 1, 1,
  1, 1, 0, 1,
  1, 1, 1, 1,
  // 0xE0
  0, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 0, 1,
  1, 1, 1, 1,
  // 0xF0
  0, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 1, 1,
  // 0x100
  1, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 1, 1,
  1, 1, 1, 1,
};

static void checkDitherSrcDest(const Image& sourceImage, Image& destImage)
{
  if (sourceImage.format() != ImageFormat::RGBA)
  {
    throw std::invalid_argument("Source image format must be RGBA");
  }

  if (destImage.format() != ImageFormat::InkyBW && 
      destImage.format() != ImageFormat::InkyBWR && 
      destImage.format() != ImageFormat::InkyBWY)
  {
    throw std::invalid_argument("Dest image format must be an Inky format!");
  }

  if (destImage.height() != sourceImage.height() || 
      destImage.width() != sourceImage.width())
  {
    throw std::invalid_argument("Dest image and source images must have the same dimensions!");
  }
}

void patternDither(const Image& sourceImage, Image& destImage)
{
  checkDitherSrcDest(sourceImage, destImage);

  int width = sourceImage.width();
  int height = sourceImage.height();
  const RGBAColor* dataRGBA = (const RGBAColor*)sourceImage.data();
  uint8_t* dataInky = destImage.data();

  // Iterate over all the color data, just converting to black and white
  for (int y=0; y < height; ++y)
  {
    for (int x=0; x < width; ++x)
    {
      int lutOffset = ((int)dataRGBA->getGrayValue() + 0x08) & 0x1F0;
      if (ditherLut[lutOffset + (y%4)*4+(x%4)])
      {
        (*dataInky) = (uint8_t)InkyColor::White;
      }
      else
      {
        (*dataInky) = (uint8_t)InkyColor::Black;
      }
      ++dataInky;
      ++dataRGBA;
    }
  }

    // // Add red accents
    // if (format == ImageFormat::InkyBWR)
    // {
    //   for (int i=0; i < dataInky.size(); ++i)
    //   {
    //     HSVColor c = dataColor[i].toHSV();
    //     if ((c.H > 340.0f || c.H < 20.0f ) && (c.S > 0.5f) && (c.V > 0.25f))
    //     {
    //       dataInky[i] = (uint8_t)InkyColor::Red;
    //     }
    //   }
    // }
    
    // // Add yellow accents
    // if (format == ImageFormat::InkyBWY)
    // {
    //   for (int i=0; i < dataInky.size(); ++i)
    //   {
    //     HSVColor c = dataColor[i].toHSV();
    //     if ((c.H > 20.0f && c.H < 80.0f ) && (c.S > 0.5f) && (c.V > 0.25f))
    //     {
    //       dataInky[i] = (uint8_t)InkyColor::Yellow;
    //     }
    //   }
    // }
}

void diffusionDither(const Image& sourceImage, Image& destImage, float saturationBias)
{
  checkDitherSrcDest(sourceImage, destImage);

  int width = sourceImage.width();
  int height = sourceImage.height();
  int size = width*height;
  
  // Copy the source image to a float array for convenience
  // (the algorithm later will want to treat the values as floats)
  std::vector<float> pixels(size);
  {
    const RGBAColor* dataRGBA = (const RGBAColor*)sourceImage.data();
    for (int i=0; i < pixels.size(); ++i)
    {
      pixels[i] = (float)dataRGBA[i].getGrayValue();
    }
  }

  // Convert to Inky BW using Floyd-Steinberg
  uint8_t* dataInky = destImage.data();
  for (int y=0; y < height; ++y)
  {
    for (int x=0; x < width; ++x)
    {
      float oldpixel = pixels[x+y*width];
      float quant_error = 0;
      if (oldpixel > 128.0f)
      {
        (*dataInky) = (uint8_t)InkyColor::White;
        quant_error = oldpixel - 255.0f + saturationBias;
      }
      else
      {
        (*dataInky) = (uint8_t)InkyColor::Black;
        quant_error = oldpixel - 0.0f - saturationBias;
      }
      
      if (x < width-1)
        pixels[ (x+1)+(y)*width ] += quant_error * (7.0f / 16.0f);
      if (x > 0 && y < height-1)
        pixels[ (x-1)+(y+1)*width ] += quant_error * (3.0f / 16.0f);
      if (y < height-1)
        pixels[ (x)+(y+1)*width ] += quant_error * (5.0f / 16.0f);
      if (x < width-1 && y < height-1)
        pixels[ (x+1)+(y+1)*width ] += quant_error * (1.0f / 16.0f);
      
      ++dataInky;
    }
  }
}

void fixedThresh(const Image& sourceImage, Image& destImage, uint8_t thresh)
{
  checkDitherSrcDest(sourceImage, destImage);

  int width = sourceImage.width();
  int height = sourceImage.height();
  const RGBAColor* dataRGBA = (const RGBAColor*)sourceImage.data();
  uint8_t* dataInky = destImage.data();

  // Iterate over all the color data, just converting to black and white
  for (int y=0; y < height; ++y)
  {
    for (int x=0; x < width; ++x)
    {
      if (dataRGBA->getGrayValue() > thresh)
      {
        (*dataInky) = (uint8_t)InkyColor::White;
      }
      else
      {
        (*dataInky) = (uint8_t)InkyColor::Black;
      }
      ++dataInky;
      ++dataRGBA;
    }
  }
}
