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
  std::cout << "\tPCB Variant: " << (int)display.pcbVariant() << std::endl;
  std::cout << "\tDisplay Variant: " << display.displayVariant() << std::endl;
  std::cout << "\tWrite Time: " << display.writeTime() << std::endl;

  if (argc == 2)
  {
    Image img;
    std::string baseName = argv[1];
    std::cout << "Reading input png..." << std::endl;
    img.readPng(baseName);
    std::cout << "Setting and converting image..." << std::endl;
    display.setImage(img);
    std::cout << "Updating display..." << std::endl;
    display.show();
    std::cout << "Done!" << std::endl;
  }
  else
  {
    std::cout << "Please provide 1 argument, the input file name to show on the display." << std::endl;
  }
}
