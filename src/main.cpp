#include "Image.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    Image img;
    std::string baseName = argv[1];
    std::cout << "Reading input png..." << std::endl;
    img.readPng(baseName + ".png");
    std::cout << "Scaling..." << std::endl;
    img.scale(400, 300, ImageScaleMode::Fill);
    img.writePng(baseName + "_scaled.png");
    std::cout << "Converting..." << std::endl;
    Image bw, bwr, bwy;
    img.convert(ImageFormat::InkyBW, ImageConvertMode::Threshold, &bw);
    img.convert(ImageFormat::InkyBWR, ImageConvertMode::Threshold, &bwr);
    img.convert(ImageFormat::InkyBWY, ImageConvertMode::Threshold, &bwy);
    bw.writePng(baseName + "_bw.png");
    bwr.writePng(baseName + "_bwr.png");
    bwy.writePng(baseName + "_bwy.png");
    std::cout << "Done!" << std::endl;
  }
  else
  {
    std::cout << "Please provide 2 arguments, input and output png file names." << std::endl;
  }
}