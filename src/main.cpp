#include "Inky.hpp"
#include "HttpService.hpp"
#include "Gpio.hpp"
#include "Image.hpp"
#include "ImageIO.hpp"
#include "Text.hpp"
#include "QRCode.hpp"
#include <magic_enum.hpp>
#include <iostream>
#include <fmt/format.h>

#include <signal.h>

using namespace magic_enum::ostream_operators;
using namespace httplib;

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

  auto display = Inky::Create();
  std::cout << "Connected to Inky Display!" << std::endl;
  std::cout << "\tWidth: " << display->info().width << std::endl;
  std::cout << "\tHeight: " << display->info().height << std::endl;
  std::cout << "\tColor Capability: " << display->info().colorCapability << std::endl;
  std::cout << "\tPCB Variant: " << (int)display->info().pcbVariant << std::endl;
  std::cout << "\tDisplay Variant: " << display->info().displayVariant << std::endl;
  std::cout << "\tWrite Time: " << display->info().writeTime << std::endl;



  HttpService http;

  Gpio gpio;
  gpio.setupLine(5, Gpio::LineMode::Input, Gpio::LineBias::PullUp);
  gpio.subscribe(5, [&](int line, Gpio::LineTransition transition, std::chrono::steady_clock::time_point timestamp)
  {
    if (transition == Gpio::LineTransition::FallingEdge)
    {
      // Get the URL to show from the HttpService
      std::string configURL = fmt::format("http://{}", http.ListeningInterface());
      auto qrCode = QRCode::GenerateImage(configURL);
      qrCode.scale(display->info().width, display->info().height-50, {.scaleMode = ScaleMode::Fit, .interpolationMode = InterpolationMode::Nearest});
      qrCode.crop(0, 0, display->info().width, display->info().height);
      Text::Draw(configURL, qrCode, display->info().width / 2, display->info().height-50, {.font = Text::Font::Mono_8x12, .alignment = Text::Alignment::Center});
      Text::Draw("Scan the QR code to upload a new photo.", qrCode, display->info().width / 2, display->info().height-30, {.font = Text::Font::Mono_8x12, .alignment = Text::Alignment::Center});
      display->setImage(qrCode);
      display->show();
    }
  });

  http.Server().Get("/current_photo.png",
  [&](const Request &req, Response &res) 
  {
    std::string imgBuf;
    ImageIO::SaveToBuffer(display->getImage(), imgBuf, {.saveFormat = ImageFormat::PNG});
    res.set_content(imgBuf, "image/png");
  });

  http.Server().Post("/post_photo",
  [&](const Request &req, Response &res) 
  {
    if (req.is_multipart_form_data()) 
    {
        // Process images here...
        std::cout << "Got " << req.files.size() << " files in multipart request!" << std::endl; 
        for (const auto& [name, data] : req.files)
        {
            std::cout << "Name: " << data.name 
                      << " | Filename: " << data.filename 
                      << " | Content Type: " << data.content_type
                      << " | Content Size: " << data.content.size()
                      << std::endl;
            if (data.content_type.find("image/") == 0)
            {
                std::cout << "Image appears to be an image, sending to display..." << std::endl;
                Image newImage = ImageIO::LoadFromBuffer(data.content);
                newImage.scale(display->info().width, display->info().height, {.scaleMode = ScaleMode::Fill});
                display->setImage(newImage);
                display->show();
                break;
            }
        }
        res.set_redirect("/");
    }
    else 
    {
    //   std::string contentType = "UNDEFINED";
    //   if (req.has_header("Content-Type")) 
    //   {
    //     contentType = req.get_header_value("Content-Type");
    //   }

    //   // Process image here...
    //   std::cout << "Got simple post request!" << std::endl;
    //   std::cout << "Content Type: " << contentType
    //             << " | Body Size: " << req.body.size()
    //             << std::endl;
    //   std::cout << "Printing first 255 chars of body:" << std::endl; 
    //   std::cout << req.body.substr(0, 255) << std::endl; 
    }
  });

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
