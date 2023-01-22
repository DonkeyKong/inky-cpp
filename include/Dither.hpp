#pragma once

#include "Image.hpp"

void patternDither(const Image& sourceImage, Image& destImage);
void diffusionDither(const Image& sourceImage, Image& destImage, float ditherAccuracy = 1.0f);
