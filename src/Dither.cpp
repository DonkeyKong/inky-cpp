#include "Dither.hpp"
#include <limits>
#include <stdexcept>
#include <map>

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
}

static InkyColor nearestInkyColor(const LabColor& color, LabColor& error, ImageFormat format)
{
  static LabColor red {54, 81, 70};
  static LabColor yellow {98, -16, 93};
  static LabColor white {100, 0, 0};
  static LabColor black {0, 0, 0};

  float dEW = white.deltaE(color);
  float dEB = black.deltaE(color);
  float dER = format == ImageFormat::InkyBWR ? red.deltaE(color) : std::numeric_limits<float>::infinity();
  float dEY = format == ImageFormat::InkyBWY ? yellow.deltaE(color) : std::numeric_limits<float>::infinity();

  if (format == ImageFormat::InkyBWR && dER <= dEW && dER <= dEB)
  {
    error = color - red;
    return InkyColor::Red;
  }
  else if (format == ImageFormat::InkyBWY && dEY <= dEW && dEY <= dEB)
  {
    error = color - yellow;
    return InkyColor::Yellow;
  }
  else if (dEW <= dEB)
  {
    error = color - white;
    return InkyColor::White;
  }
  else
  {
    error = color - black;
    return InkyColor::Black;
  }
}

static std::vector<LabColor> getLabVector(const Image& sourceImage, ImageFormat destFormat)
{
  if (sourceImage.format() != ImageFormat::RGBA)
  {
    throw std::invalid_argument("Source image format must be RGBA");
  }

  if (destFormat != ImageFormat::InkyBW && 
      destFormat != ImageFormat::InkyBWR && 
      destFormat != ImageFormat::InkyBWY)
  {
    throw std::invalid_argument("Dest image format must be an Inky format!");
  }

  // Copy the source image to a float array for convenience
  // (the algorithm later will want to treat the values as floats)
  std::vector<LabColor> labVals(sourceImage.width()*sourceImage.height());
  
  const RGBAColor* dataRGBA = (const RGBAColor*)sourceImage.data();
  if (destFormat == ImageFormat::InkyBW)
  {
    // For black and white output, we will get a higher quality result
    // if we first convert to grayscale, then dither the gray image.
    for (int i=0; i < labVals.size(); ++i)
    {
      uint8_t g = dataRGBA[i].getGrayValue();
      labVals[i] = RGBAColor{g,g,g,255}.toLab();
    }
  }
  else
  {
    // Yellow and red accents work best right on the color image
    for (int i=0; i < labVals.size(); ++i)
    {
      labVals[i] = dataRGBA[i].toLab();
    }
  }
  
  return labVals;
}

void diffuseError(std::vector<LabColor>& labVals, const LabColor& oldValue, const LabColor& error, const int x, const int y, const int width, const int height)
{
  if (x < width-1)                                           // Traditional weights...
    labVals[ (x+1)+(y)*width ] += error *   (5.0f / 16.0f);  // 7
  if (x > 0 && y < height-1)
    labVals[ (x-1)+(y+1)*width ] += error * (3.0f / 16.0f);  // 3
  if (y < height-1)
    labVals[ (x)+(y+1)*width ] += error *   (5.0f / 16.0f);  // 5
  if (x < width-1 && y < height-1)
    labVals[ (x+1)+(y+1)*width ] += error * (3.0f / 16.0f);  // 1
}

template <typename T> 
T sign(T val) 
{
    return (T(0) < val) - (val < T(0));
}

void diffuseErrorGradientInd(std::vector<LabColor>& labVals, const LabColor& oldValue, const LabColor& error, const int x, const int y, const int width, const int height)
{
  //static const float weights[] = {(1.0f / 16.0f), (3.0f / 16.0f), (5.0f / 16.0f), (7.0f / 16.0f)};
  static const float weights[] = {(7.0f / 16.0f), (5.0f / 16.0f), (3.0f / 16.0f), (1.0f / 16.0f)};


  if (x == width-1 || x == 0 || y == height-1 || y == 0)
  {
    diffuseError(labVals, oldValue, error, x, y, width, height);
  }
  else
  {
    // Fetch refs to the error candidates
    LabColor& labE = labVals[ (x+1)+(y)*width ];
    LabColor& labSW = labVals[ (x-1)+(y+1)*width ];
    LabColor& labS = labVals[ (x)+(y+1)*width ];
    LabColor& labSE = labVals[ (x+1)+(y+1)*width ];

    // Calculate gradient to the error candidates
    LabColor errorSign = {sign(error.L), sign(error.a), sign(error.b)};
    LabColor eE = (oldValue - labE) * errorSign;
    LabColor eSW = (oldValue - labSW) * errorSign;
    LabColor eS = (oldValue - labS) * errorSign;
    LabColor eSE = (oldValue - labSE) * errorSign;

    // Construct a sorted map for each channel
    std::multimap<float, LabColor*> sortedErrorsL
    {
      {eE.L, &labE},
      {eSW.L, &labSW},
      {eS.L, &labS},
      {eSE.L, &labSE}
    };
    int i = 0;
    for (auto& [mag, color] : sortedErrorsL)
    {
      color->L += weights[i] * error.L;
      ++i;
    }

    std::multimap<float, LabColor*> sortedErrorsA
    {
      {eE.a, &labE},
      {eSW.a, &labSW},
      {eS.a, &labS},
      {eSE.a, &labSE}
    };
    i = 0;
    for (auto& [mag, color] : sortedErrorsA)
    {
      color->a += weights[i] * error.a;
      ++i;
    }

    std::multimap<float, LabColor*> sortedErrorsB
    {
      {eE.b, &labE},
      {eSW.b, &labSW},
      {eS.b, &labS},
      {eSE.b, &labSE}
    };
    i = 0;
    for (auto& [mag, color] : sortedErrorsB)
    {
      color->b += weights[i] * error.b;
      ++i;
    }
  }
}

void diffuseErrorGradient(std::vector<LabColor>& labVals, const LabColor& oldValue, const LabColor& error, const int x, const int y, const int width, const int height)
{
  if (x == width-1 || x == 0 || y == height-1 || y == 0)
  {
    diffuseError(labVals, oldValue, error, x, y, width, height);
  }
  else
  {
    //static const float weights[] = {(1.0f / 16.0f), (3.0f / 16.0f), (5.0f / 16.0f), (7.0f / 16.0f)};
    static const float weights[] = {(8.0f / 16.0f), (5.0f / 16.0f), (3.0f / 16.0f), (1.0f / 16.0f)};

    // Fetch refs to the error candidates
    LabColor& labE = labVals[ (x+1)+(y)*width ];
    //LabColor& labSW = labVals[ (x-1)+(y+1)*width ];
    LabColor& labS = labVals[ (x)+(y+1)*width ];
    LabColor& labSE = labVals[ (x+1)+(y+1)*width ];

    // Calculate gradient to the error candidates
    LabColor errorSign = {sign(error.L), sign(error.a), sign(error.b)};
    LabColor eE = (oldValue - labE) * errorSign;
    //LabColor eSW = (oldValue - labSW) * errorSign;
    LabColor eS = (oldValue - labS) * errorSign;
    LabColor eSE = (oldValue - labSE) * errorSign;

    // Construct a sorted map for each channel
    std::multimap<float, LabColor*> sortedErrorsL
    {
      {eE.L  /*+ eE.a + eE.b  */, &labE},
      //{eSW.L /*+ eSW.a + eSW.b*/, &labSW},
      {eS.L  /*+ eS.a + eS.b  */, &labS},
      {eSE.L /*+ eSE.a + eSE.b*/, &labSE}
    };
    int i = 0;
    for (auto& [mag, color] : sortedErrorsL)
    {
      (*color) += error * weights[i];
      ++i;
    }
  }
}

void diffusionDither(const Image& sourceImage, Image& destImage, float ditherAccuracy)
{
  checkDitherSrcDest(sourceImage, destImage);

  int width = sourceImage.width();
  int height = sourceImage.height();
  ImageFormat format = destImage.format();
  
  // Get the source image data in LAB color format
  std::vector<LabColor> labVals = getLabVector(sourceImage, destImage.format());

  // Convert to Inky using Floyd-Steinberg dithering
  uint8_t* dataInky = destImage.data();
  for (int y=0; y < height; ++y)
  {
    for (int x=0; x < width; ++x)
    {
      LabColor oldValue = labVals[x+y*width];
      LabColor error;
      (*dataInky) = (uint8_t)nearestInkyColor(oldValue, error, format);
      error = error * ditherAccuracy;

      diffuseError(labVals, oldValue, error, x, y, width, height);
      
      ++dataInky;
    }
  }
}
