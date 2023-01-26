#include "Inky.hpp"
#include "HttpService.hpp"
#include "Image.hpp"
#include <magic_enum.hpp>
#include <iostream>
#include <fmt/format.h>

using namespace magic_enum::ostream_operators;

int main(int argc, char *argv[])
{
  Inky display;
  HttpService http;

  // Get the URL to show from the HttpService
  std::string configURL = fmt::format("http://{}", http.ListeningInterface());
  auto qrCode = Image::FromQrPayload(configURL);
  qrCode->scale(display.width(), display.height(), {.scaleMode = ImageScaleMode::Fit});
  display.setImage(*qrCode);
  
  // svr.Post("/content_receiver",
  // [&](const Request &req, Response &res, const ContentReader &content_reader) {
  //   if (req.is_multipart_form_data()) {
  //     // NOTE: `content_reader` is blocking until every form data field is read
  //     MultipartFormDataItems files;
  //     content_reader(
  //       [&](const MultipartFormData &file) {
  //         files.push_back(file);
  //         return true;
  //       },
  //       [&](const char *data, size_t data_length) {
  //         files.back().content.append(data, data_length);
  //         return true;
  //       });
  //   } else {
  //     std::string body;
  //     content_reader([&](const char *data, size_t data_length) {
  //       body.append(data, data_length);
  //       return true;
  //     });
  //   }
  // });
  

  // std::cout << "Connected to Inky Display!" << std::endl;
  // std::cout << "\tWidth: " << display.width() << std::endl;
  // std::cout << "\tHeight: " << display.height() << std::endl;
  // std::cout << "\tColor Capability: " << display.colorCapability() << std::endl;
  // std::cout << "\tPCB Variant: " << (int)display.pcbVariant() << std::endl;
  // std::cout << "\tDisplay Variant: " << display.displayVariant() << std::endl;
  // std::cout << "\tWrite Time: " << display.writeTime() << std::endl;

  // if (argc == 2)
  // {
  //   Image img;
  //   std::string baseName = argv[1];
  //   std::cout << "Reading input png..." << std::endl;
  //   img.readPng(baseName);
  //   std::cout << "Setting and converting image..." << std::endl;
  //   display.setImage(img);
  //   std::cout << "Updating display..." << std::endl;
  //   display.show();
  //   std::cout << "Done!" << std::endl;
  // }
  // else
  // {
  //   std::cout << "Please provide 1 argument, the input file name to show on the display." << std::endl;
  // }
}
