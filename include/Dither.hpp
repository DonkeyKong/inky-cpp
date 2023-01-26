#pragma once

#include "Image.hpp"

void patternDither(const Image& sourceImage, Image& destImage);
void diffusionDither(const Image& sourceImage, Image& destImage, float ditherAccuracy = 1.0f);
InkyColor nearestInkyColor(const LabColor& color, LabColor& error, ImageFormat format);
InkyColor nearestInkyColor(const RGBAColor& color, ImageFormat format);
InkyColor nearestInkyColor(const LabColor& color, ImageFormat format);