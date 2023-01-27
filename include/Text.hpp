#pragma once

#include "Image.hpp"

namespace Text
{
  enum class Font : int
  {
    Mono_4x6,
    Mono_6x6,
    Mono_8x12
  };

  enum class Alignment
  {
    Left,
    Center,
    Right
  };

  struct TextStyle
  {
    Font font = Font::Mono_6x6;
    Alignment alignment = Alignment::Left;
    RGBAColor color = {0,0,0,255};
  };

  void Draw(const std::string& str, Image& dest, int x, int y, TextStyle style = {});
}