#include "Text.hpp"
#include "Dither.hpp"
#include <unordered_map>

namespace Text
{

static Image loadFont(const std::string& path)
{
  static IndexedColorMap colorMap(
  {
        {ColorName::Black, 0, {0,0,0}},
        {ColorName::White, 1, {255,255,255}}
  });
  Image font = Image::FromPngFile(path);
  // Create a binarized index image of the font
  font.toIndexed(colorMap, {.ditherAccuracy = 0.0f});
  return font;
}

// Static resoures for drawing text on to images
const static std::unordered_map<Font, Image> Fonts
{
  {Font::Mono_4x6,  loadFont("resources/font_4x6.png")},
  {Font::Mono_6x6,  loadFont("resources/font_6x6.png")},
  {Font::Mono_8x12, loadFont("resources/font_8x12.png")},
};

template <typename PixelType>
static inline void characterBlit(const char character, const int x, const int y, const PixelType& color,
                                 const IndexedColor* fontData, const int charWidth, const int charHeight, const int fontWidth,
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
        if (fontData[iX+charOffsetX+charBox.x+(iY+charOffsetY+charBox.y)*fontWidth] != 0)
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
                                 (IndexedColor*)font.data(), charWidth, charHeight, font.width(),
                                 destData, destBox);
      x += charWidth;
    }
  }
  else
  {
    IndexedColor* destData = (IndexedColor*)dest.data();
    IndexedColor color = dest.colorMap().toIndexedColor(style.color);
    BoundingBox destBox = dest.bounds();
    for (const auto& ch : str)
    {
      characterBlit<IndexedColor>(ch, x, y, color,
                                (IndexedColor*)font.data(), charWidth, charHeight, font.width(),
                                destData, destBox);
      x += charWidth;
    }
  }
  
}

}