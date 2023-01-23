#include "Inky.hpp"
#include <magic_enum.hpp>
#include <iostream>

using namespace magic_enum::ostream_operators;

int main(int argc, char *argv[])
{
  Inky display;
  std::cout << "Connected to Inky Display!" << std::endl;
  std::cout << "\tWidth: " << display.width() << std::endl;
  std::cout << "\tHeight: " << display.height() << std::endl;
  std::cout << "\tColor Capability: " << display.colorCapability() << std::endl;
  std::cout << "\tPCB Variant: " << display.pcbVariant() << std::endl;
  std::cout << "\tDisplay Variant: " << display.displayVariant() << std::endl;
  std::cout << "\tWrite Time: " << display.writeTime() << std::endl;

  // if (argc == 2)
  // {
  //   Image img;
  //   std::string baseName = argv[1];
  //   std::cout << "Reading input png..." << std::endl;
  //   img.readPng(baseName + ".png");
  //   std::cout << "Scaling..." << std::endl;
  //   img.scale(400, 400, {ImageScaleMode::Fill});
  //   img.writePng(baseName + "_scaled.png");
  //   std::cout << "Converting..." << std::endl;
    
  //   Image inky;

  //   img.convert(ImageFormat::InkyBW, ImageConversionSettings{.ditherMode = DitherMode::Diffusion, .ditherAccuracy = 0.0f}, &inky);
  //   inky.writePng(baseName + "_thresh_bw.png");
  //   img.convert(ImageFormat::InkyBWR, ImageConversionSettings{.ditherMode = DitherMode::Diffusion, .ditherAccuracy = 0.0f}, &inky);
  //   inky.writePng(baseName + "_thresh_bwr.png");
  //   img.convert(ImageFormat::InkyBWY, ImageConversionSettings{.ditherMode = DitherMode::Diffusion, .ditherAccuracy = 0.0f}, &inky);
  //   inky.writePng(baseName + "_thresh_bwy.png");

  //   img.convert(ImageFormat::InkyBW, ImageConversionSettings{.ditherMode = DitherMode::Pattern}, &inky);
  //   inky.writePng(baseName + "_pattern_bw.png");
  //   img.convert(ImageFormat::InkyBWR, ImageConversionSettings{.ditherMode = DitherMode::Pattern}, &inky);
  //   inky.writePng(baseName + "_pattern_bwr.png");
  //   img.convert(ImageFormat::InkyBWY, ImageConversionSettings{.ditherMode = DitherMode::Pattern}, &inky);
  //   inky.writePng(baseName + "_pattern_bwy.png");

  //   img.convert(ImageFormat::InkyBW, ImageConversionSettings{.ditherMode = DitherMode::Diffusion}, &inky);
  //   inky.writePng(baseName + "_diffusion_bw.png");

  //   for (float a = 0; a < 1.0f; a+=0.025)
  //   {
  //     img.convert(ImageFormat::InkyBWR, ImageConversionSettings{.ditherMode = DitherMode::Diffusion, .ditherAccuracy = a}, &inky);
  //     inky.writePng(baseName + "_diffusion_bwr_" + std::to_string(a) + ".png");
  //   }


  //   img.convert(ImageFormat::InkyBWY, ImageConversionSettings{.ditherMode = DitherMode::Diffusion}, &inky);
  //   inky.writePng(baseName + "_diffusion_bwy.png");

  //   std::cout << "Done!" << std::endl;
  // }
  // else
  // {
  //   std::cout << "Please provide 1 argument, the input file name with no extension." << std::endl;
  // }
}
