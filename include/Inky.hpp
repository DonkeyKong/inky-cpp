#pragma once

#include "Image.hpp"

class Inky
{
public:
  Inky();
  ~Inky();
  void setImage(Image image);
  void setBorder(InkyColor color);
  void show();
};
