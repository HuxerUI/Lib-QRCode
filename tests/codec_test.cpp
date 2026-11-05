#include <huxerui/qrcode/codec.h>

#include "test.h"
#include "fixtures/qr_reference.h"

#include <algorithm>
#include <limits>
#include <thread>

using namespace huxerui::qrcode;

void RoundTrip(std::string_view text, EncodeOptions options = {}) {
  const auto symbol = Encode(text, options);
  CHECK(symbol.Width() > 0 && symbol.Height() > 0);
  CHECK(symbol.Modules().size() == static_cast<std::size_t>(symbol.Width()) * symbol.Height());
  CHECK(symbol.Format() == options.format);
  const auto image = symbol.Rasterize(3);
  const auto decoded = Decode(image.View(), {.formats = {options.format}});
  CHECK(decoded.has_value());
  CHECK(decoded->format == options.format);
  if(decoded->text != text)
    std::cerr << FormatName(options.format) << " expected [" << text << "] (" << text.size()
              << ") decoded [" << decoded->text << "] (" << decoded->text.size() << ")\n";
  CHECK(decoded->text == text);
  CHECK(!decoded->bytes.empty());
  for(const auto& point : decoded->position) {
    CHECK(point.x >= 0 && point.x < image.width);
    CHECK(point.y >= 0 && point.y < image.height);
  }
}

int main() { return Run([] {
  constexpr int fixture_size = static_cast<int>(qr_reference.size()) * 4;
  GrayImage reference{fixture_size, fixture_size, std::vector<std::uint8_t>(fixture_size * fixture_size)};
  for(int y = 0; y < fixture_size; ++y)
    for(int x = 0; x < fixture_size; ++x)
      reference.pixels[static_cast<std::size_t>(y) * fixture_size + x] =
          qr_reference[static_cast<std::size_t>(y / 4)][static_cast<std::size_t>(x / 4)] == '1' ? 0 : 255;
  const auto external = Decode(reference.View());
  CHECK(external.has_value());
  CHECK(external->text == "HuxerUI external fixture");
  CHECK(external->error_correction == "M");
  RoundTrip("https://github.com/HuxerUI");
  RoundTrip("二维码 / 日本語 / 😀");
  RoundTrip(std::string_view("A\0B\x01\x7F", 5));
  for(const auto [correction, name] : {
      std::pair{ErrorCorrection::Low, "L"}, {ErrorCorrection::Medium, "M"},
      {ErrorCorrection::Quartile, "Q"}, {ErrorCorrection::High, "H"}}) {
    const auto image = Encode("correction", {.qr_error_correction = correction}).Rasterize();
    CHECK(Decode(image.View())->error_correction == name);
  }
  for(auto format : {BarcodeFormat::Code128, BarcodeFormat::Code39, BarcodeFormat::Code93,
                      BarcodeFormat::DataMatrix, BarcodeFormat::Aztec, BarcodeFormat::PDF417})
    RoundTrip("HUXERUI-2026", {.format = format});
  for(auto format : {BarcodeFormat::DataMatrix, BarcodeFormat::Aztec, BarcodeFormat::PDF417})
    RoundTrip("中文 😀", {.format = format});
  RoundTrip(std::string_view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C", 12),
            {.format = BarcodeFormat::PDF417});
  RoundTrip("96385074", {.format = BarcodeFormat::EAN8});
  RoundTrip("5901234123457", {.format = BarcodeFormat::EAN13});
  RoundTrip("012345678905", {.format = BarcodeFormat::UPCA});
  RoundTrip("01234565", {.format = BarcodeFormat::UPCE});
  RoundTrip("12345678", {.format = BarcodeFormat::ITF});
  RoundTrip("A123456B", {.format = BarcodeFormat::Codabar});

  const auto qr = Encode("immutable");
  const auto copied = qr;
  CHECK(copied.Modules().data() == qr.Modules().data());
  CHECK(copied == qr);
  CHECK(!(Encode("immutable") == qr));
  CHECK(qr.Width() == qr.Height());
  for(int y = 0; y < qr.Height(); ++y)
    for(int x = 0; x < qr.Width(); ++x)
      if(x < 4 || y < 4 || x >= qr.Width() - 4 || y >= qr.Height() - 4) CHECK(!qr.IsDark(x, y));
  const auto raw = Encode("immutable", {.quiet_zone = 0});
  CHECK(qr.Width() == raw.Width() + 8);
  CHECK(qr.Height() == raw.Height() + 8);
  for(int y = 0; y < raw.Height(); ++y)
    for(int x = 0; x < raw.Width(); ++x) CHECK(qr.IsDark(x + 4, y + 4) == raw.IsDark(x, y));
  const auto raster = qr.Rasterize(2);
  for(int y = 0; y < raster.height; ++y)
    for(int x = 0; x < raster.width; ++x)
      CHECK(raster.pixels[static_cast<std::size_t>(y) * raster.width + x] == (qr.IsDark(x / 2, y / 2) ? 0 : 255));
  CHECK(Decode(raster.View(), {.pure = true})->text == "immutable");
  CHECK(!Decode(raster.View(), {.formats = {BarcodeFormat::Code128}}));
  auto inverted = raster;
  for(auto& pixel : inverted.pixels) pixel = 255 - pixel;
  CHECK(Decode(inverted.View())->text == "immutable");
  auto rotated = raster;
  for(int y = 0; y < raster.height; ++y)
    for(int x = 0; x < raster.width; ++x)
      rotated.pixels[static_cast<std::size_t>(x) * raster.height + raster.height - 1 - y] =
          raster.pixels[static_cast<std::size_t>(y) * raster.width + x];
  CHECK(Decode(rotated.View())->text == "immutable");

  // Exercise padded color buffers; final-row padding is deliberately omitted.
  for(auto format : {PixelFormat::RGB8, PixelFormat::BGR8, PixelFormat::RGBA8, PixelFormat::BGRA8}) {
    const int channels = format == PixelFormat::RGB8 || format == PixelFormat::BGR8 ? 3 : 4;
    const int stride = raster.width * channels + 7;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(stride) * (raster.height - 1) + raster.width * channels, 17);
    for(int y = 0; y < raster.height; ++y) {
      for(int x = 0; x < raster.width; ++x) {
        auto* pixel = pixels.data() + static_cast<std::size_t>(y) * stride + x * channels;
        const bool light = raster.pixels[static_cast<std::size_t>(y) * raster.width + x] != 0;
        pixel[0] = light ? 255 : 20;
        pixel[1] = light ? 245 : 0;
        pixel[2] = light ? 240 : 10;
        if(channels == 4) pixel[3] = 255;
      }
    }
    CHECK(Decode({pixels, raster.width, raster.height, format, stride})->text == "immutable");
  }
  const auto left = Encode("left").Rasterize();
  const auto right = Encode("right").Rasterize();
  GrayImage combined{left.width + right.width + 40, std::max(left.height, right.height), {}};
  combined.pixels.resize(static_cast<std::size_t>(combined.width) * combined.height, 255);
  for(int y = 0; y < left.height; ++y)
    std::copy_n(left.pixels.data() + y * left.width, left.width, combined.pixels.data() + y * combined.width);
  for(int y = 0; y < right.height; ++y)
    std::copy_n(right.pixels.data() + y * right.width, right.width,
                combined.pixels.data() + y * combined.width + left.width + 40);
  const auto both = DecodeAll(combined.View());
  CHECK(both.size() == 2);
  CHECK(std::any_of(both.begin(), both.end(), [](const auto& code) { return code.text == "left"; }));
  CHECK(std::any_of(both.begin(), both.end(), [](const auto& code) { return code.text == "right"; }));
  CHECK(DecodeAll(combined.View(), {.max_symbols = 1}).size() == 1);
  GrayImage white{100, 100, std::vector<std::uint8_t>(10000, 255)};
  CHECK(!Decode(white.View()));
  CHECK(DecodeAll(white.View()).empty());

  const auto svg = qr.ToSvg();
  CHECK(svg.starts_with("<svg xmlns=\"http://www.w3.org/2000/svg\""));
  CHECK(svg.ends_with("</svg>"));
  CHECK(svg.find("crispEdges") != std::string::npos);
  CHECK(svg.find("immutable") == std::string::npos);
  CHECK(qr.ToSvg({.foreground = 0x123456}).find("#123456") != std::string::npos);
  CHECK(FormatName(BarcodeFormat::QRCode) == "QR Code");
  for(int i = 0; i < 20; ++i) CHECK(!FormatName(static_cast<BarcodeFormat>(i)).empty());
  CHECK(CanEncode(BarcodeFormat::Code128));
  CHECK(!CanEncode(BarcodeFormat::MicroQRCode));
  CHECK(!CanEncode(static_cast<BarcodeFormat>(-1)));

  Throws<std::out_of_range>([&] { (void)qr.IsDark(-1, 0); });
  Throws<std::out_of_range>([&] { (void)qr.IsDark(0, qr.Height()); });
  Throws<std::invalid_argument>([] { (void)Encode(""); });
  for(auto text : {std::string("\xC0\xAF"), std::string("\xED\xA0\x80"), std::string("\xF4\x90\x80\x80"),
                   std::string("\xE2\x82"), std::string("\x80"), std::string("\xE2\x28\xA1")})
    Throws<std::invalid_argument>([&] { (void)Encode(text); });
  Throws<std::invalid_argument>([] { (void)Encode(std::string(8193, 'x')); });
  Throws<std::invalid_argument>([] { (void)Encode(std::string(5000, 'x')); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.format = BarcodeFormat::MicroQRCode}); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.format = static_cast<BarcodeFormat>(99)}); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.quiet_zone = -2}); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.quiet_zone = 257}); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.bar_height = 0}); });
  Throws<std::invalid_argument>([] { (void)Encode("x", {.qr_error_correction = static_cast<ErrorCorrection>(99)}); });
  Throws<std::invalid_argument>([] { (void)Encode("not digits", {.format = BarcodeFormat::EAN13}); });
  Throws<std::invalid_argument>([] { (void)Encode("5901234123450", {.format = BarcodeFormat::EAN13}); });
  Throws<std::invalid_argument>([] { (void)Encode("123", {.format = BarcodeFormat::ITF}); });
  Throws<std::invalid_argument>([&] { (void)qr.Rasterize(0); });
  Throws<std::invalid_argument>([&] { (void)qr.Rasterize(std::numeric_limits<int>::max()); });
  Throws<std::invalid_argument>([&] { (void)qr.Rasterize(1000); });
  Throws<std::invalid_argument>([&] { (void)qr.ToSvg({.scale = -1}); });
  Throws<std::invalid_argument>([&] { (void)qr.ToSvg({.foreground = 0xFF000000}); });
  Throws<std::invalid_argument>([] { (void)Decode({}); });
  Throws<std::invalid_argument>([] { (void)Decode({{}, 10, 10}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, -1, 10}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, 1, 1, static_cast<PixelFormat>(99)}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, 10, 10, PixelFormat::RGBA8, 39}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, 10, 10, PixelFormat::Gray8, -10}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, 1, 2, PixelFormat::Gray8, std::numeric_limits<int>::max()}); });
  Throws<std::invalid_argument>([&] { (void)Decode({raster.pixels, 100000, 100000}); });
  Throws<std::invalid_argument>([&] { (void)Decode(raster.View(), {.max_symbols = 0}); });
  Throws<std::invalid_argument>([&] { (void)DecodeAll(raster.View(), {.max_symbols = 256}); });
  Throws<std::invalid_argument>([&] { (void)Decode(raster.View(), {.formats = {static_cast<BarcodeFormat>(99)}}); });

  std::array<bool, 4> passed{};
  std::vector<std::thread> workers;
  for(std::size_t i = 0; i < passed.size(); ++i) {
    workers.emplace_back([&, i] {
      const auto image = copied.Rasterize();
      const auto result = Decode(image.View());
      passed[i] = result && result->text == "immutable";
    });
  }
  for(auto& worker : workers) worker.join();
  CHECK(std::all_of(passed.begin(), passed.end(), [](bool value) { return value; }));
}); }
