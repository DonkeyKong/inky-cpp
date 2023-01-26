#include "Inky.hpp"
#include "HttpService.hpp"
#include "Image.hpp"
#include "ImageText.hpp"
#include <magic_enum.hpp>
#include <iostream>
#include <fmt/format.h>

#include <signal.h>

using namespace magic_enum::ostream_operators;

volatile bool interrupt_received = false;
volatile bool internal_exit = false;

static void InterruptHandler(int signo)
{
    interrupt_received = true;
}

int main(int argc, char *argv[])
{
  // Subscribe to signal interrupts
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  Inky display;
  HttpService http;

  // Get the URL to show from the HttpService
  std::string configURL = fmt::format("http://{}", http.ListeningInterface());
  auto qrCode = Image::FromQrPayload(configURL);
  qrCode.scale(display.width(), display.height()-50, {.scaleMode = ImageScaleMode::Fit, .interpolationMode = ImageInterpolationMode::Nearest});
  qrCode.crop(0, 0, display.width(), display.height());
  Text::Draw(configURL, qrCode, 200, 250, {.font = Text::Font::Mono_8x12, .alignment = Text::Alignment::Center});
  Text::Draw("Scan the QR code to upload a new photo.", qrCode, 200, 270, {.font = Text::Font::Mono_8x12, .alignment = Text::Alignment::Center});
  display.setImage(qrCode);
  display.show();
  
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

  // Start the main waiting loop
  while (!interrupt_received && !internal_exit)
  {
      // Regulate update rate
      std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (interrupt_received)
  {
      fprintf(stderr, "Main thread caught exit signal.\n");
  }

  if (internal_exit)
  {
      fprintf(stderr, "Main thread got internal exit request.\n");
  }
}
