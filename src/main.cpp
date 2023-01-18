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
    img.scale(400, 400, {ImageScaleMode::Fill});
    img.writePng(baseName + "_scaled.png");
    std::cout << "Converting..." << std::endl;
    Image bw;
    img.convert(ImageFormat::InkyBW, ImageConversionSettings(), &bw);
    bw.writePng(baseName + "_bw.png");
    std::cout << "Done!" << std::endl;
  }
  else
  {
    std::cout << "Please provide 1 argument, the input file name with no extension." << std::endl;
  }
}
