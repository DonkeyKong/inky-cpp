#pragma once

#include "Image.hpp"

void patternDither(const Image& sourceImage, Image& destImage);
void diffusionDither(const Image& sourceImage, Image& destImage, float saturationBias = 20.0f);
void fixedThresh(const Image& sourceImage, Image& destImage, uint8_t thresh = 80);