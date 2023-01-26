#include "ImageText.hpp"
#include "Dither.hpp"
#include <map>

namespace Text
{
// Static resoures for drawing text on to images
const static std::map<Font, Image> Fonts
{
  {Font::Mono_4x6,  Image::FromPngFile("resources/font_4x6.png", ImageFormat::InkyBW, {.ditherAccuracy = 0.0f})  },
  {Font::Mono_6x6,  Image::FromPngFile("resources/font_6x6.png", ImageFormat::InkyBW, {.ditherAccuracy = 0.0f})  },
  {Font::Mono_8x12, Image::FromPngFile("resources/font_8x12.png", ImageFormat::InkyBW, {.ditherAccuracy = 0.0f}) },
};

template <typename PixelType>
static inline void characterBlit(const char character, const int x, const int y, const PixelType& color,
                                 const InkyColor* fontData, const int charWidth, const int charHeight, const int fontWidth,
                                 PixelType* destData, BoundingBox& destBox)
{
  // Create a bounding box for the character we want to blit
  BoundingBox charBox {0, 0, charWidth, charHeight};
  int charOffsetX = (character % 16) * charWidth;
  int charOffsetY = (character / 16) * charHeight;

  // Move the dest box so that the charBox is in the right place on the image
  destBox.x = -x;
  destBox.y = -y;

  // Clip the char box to the dest box
  charBox.clipTo(destBox);

  // Check if there's any blittable area left
  if (charBox.width > 0 && charBox.height > 0)
  {
    // Now it should be safe to blindly blit the charBox on to destData
    for (int iY = 0; iY < charBox.height; ++iY)
    {
      for (int iX = 0; iX < charBox.width; ++iX)
      {
        if (fontData[iX+charOffsetX+charBox.x+(iY+charOffsetY+charBox.y)*fontWidth] == InkyColor::White)
        {
          destData[iX+charBox.x+x+(iY+charBox.y+y)*destBox.width] = color;
        }
      }
    }
  }
}

void Draw(const std::string& str, Image& dest, int x, int y, TextStyle style)
{
  const Image& font = Fonts.at(style.font);
  int charWidth = font.width() / 16;
  int charHeight = font.height() / 16;

  if (style.alignment == Alignment::Center)
  {
    x -= ((str.size() * charWidth) / 2);
  }
  else if (style.alignment == Alignment::Right)
  {
    x -= (str.size() * charWidth);
  }

  if (dest.format() == ImageFormat::RGBA)
  {
    RGBAColor* destData = (RGBAColor*)dest.data();
    BoundingBox destBox = dest.bounds();
    for (const auto& ch : str)
    {
      characterBlit<RGBAColor>(ch, x, y, style.color,
                                 (InkyColor*)font.data(), charWidth, charHeight, font.width(),
                                 destData, destBox);
      x += charWidth;
    }
  }
  else
  {
    InkyColor* destData = (InkyColor*)dest.data();
    InkyColor color = nearestInkyColor(style.color, dest.format());
    BoundingBox destBox = dest.bounds();
    for (const auto& ch : str)
    {
      characterBlit<InkyColor>(ch, x, y, color,
                                (InkyColor*)font.data(), charWidth, charHeight, font.width(),
                                destData, destBox);
      x += charWidth;
    }
  }
  
}

}