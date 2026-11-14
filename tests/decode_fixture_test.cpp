#include <huxerui/qrcode/codec.h>

#include "test.h"

#include <fstream>

using namespace huxerui::qrcode;

namespace {

GrayImage LoadFixture(std::string_view name) {
  const auto path = std::string(QRCODE_FIXTURE_DIR) + "/" + std::string(name) + ".pgm";
  std::ifstream file(path, std::ios::binary);
  if(!file) throw std::runtime_error("Could not open fixture: " + path);
  std::string magic;
  int width = 0, height = 0, maximum = 0;
  file >> magic >> width >> height >> maximum;
  CHECK(magic == "P5");
  CHECK(width > 0 && height > 0 && width <= 2048 && height <= 2048 && maximum == 255);
  CHECK(file.get() == '\n');
  GrayImage image{width, height, std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height)};
  file.read(reinterpret_cast<char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size()));
  CHECK(file.gcount() == static_cast<std::streamsize>(image.pixels.size()));
  CHECK(file.peek() == std::char_traits<char>::eof());
  return image;
}

void CheckResult(const DecodeResult& result, BarcodeFormat format, std::string_view text, const GrayImage& image) {
  CHECK(result.format == format);
  CHECK(result.text == text);
  CHECK(std::string(result.bytes.begin(), result.bytes.end()) == text);
  CHECK(!result.symbology_identifier.empty());
  for(const auto& point : result.position) {
    CHECK(point.x >= 0 && point.x < image.width);
    CHECK(point.y >= 0 && point.y < image.height);
  }
}

} // namespace

int main() { return Run([] {
  struct Fixture {
    std::string_view name;
    BarcodeFormat format;
    std::string_view text;
  };
  constexpr Fixture fixtures[]{
    {"micro_qr", BarcodeFormat::MicroQRCode, "1234567"},
    {"rmqr", BarcodeFormat::RMQRCode, "HUXERUI-RMQR"},
    {"maxicode", BarcodeFormat::MaxiCode, "HUXERUI-MAXICODE"},
    {"databar", BarcodeFormat::DataBar, "0109501101530003"},
    {"databar_limited", BarcodeFormat::DataBarLimited, "0109501101530003"},
    {"databar_stacked", BarcodeFormat::DataBar, "0109501101530003"},
    {"databar_stacked_omni", BarcodeFormat::DataBar, "0109501101530003"},
    {"databar_expanded", BarcodeFormat::DataBarExpanded, "010950110153000310ABC123"},
    {"databar_expanded_stacked", BarcodeFormat::DataBarExpanded, "010950110153000310ABC123"},
    {"compact_pdf417", BarcodeFormat::PDF417, "HUXERUI-COMPACT-PDF417"},
    {"code39_extended", BarcodeFormat::Code39, "HuxerUI-2026"},
    {"dx_film_edge", BarcodeFormat::DXFilmEdge, "112-1/10A"},
    {"upca", BarcodeFormat::UPCA, "012345678905"},
    {"upce", BarcodeFormat::UPCE, "01234565"},
  };
  for(const auto& fixture : fixtures) {
    const auto image = LoadFixture(fixture.name);
    std::cout << "Checking " << fixture.name << '\n';
    for(bool pure : {false, true}) {
      const auto filtered = DecodeAll(image.View(), {.formats = {fixture.format}, .pure = pure});
      if(!CanDecode(fixture.format)) {
        CHECK(filtered.empty());
        continue;
      }
      CHECK(filtered.size() == 1);
      CheckResult(filtered.front(), fixture.format, fixture.text, image);
      const auto unfiltered = DecodeAll(image.View(), {.pure = pure});
      CHECK(unfiltered.size() == 1);
      CheckResult(unfiltered.front(), fixture.format, fixture.text, image);
      CHECK(!Decode(image.View(), {.formats = {BarcodeFormat::Code128}, .pure = pure}));
      if(fixture.format == BarcodeFormat::MicroQRCode || fixture.format == BarcodeFormat::RMQRCode)
        CHECK(!Decode(image.View(), {.formats = {BarcodeFormat::QRCode}, .pure = pure}));
    }
  }
  if(CanDecode(BarcodeFormat::UPCA)) {
    const auto upca = LoadFixture("upca");
    const auto ean = Decode(upca.View(), {.formats = {BarcodeFormat::EAN13}});
    CHECK(ean.has_value());
    CheckResult(*ean, BarcodeFormat::EAN13, "0012345678905", upca);
    const auto mixed = Decode(upca.View(), {.formats = {BarcodeFormat::EAN13, BarcodeFormat::UPCA}});
    CHECK(mixed.has_value());
    CheckResult(*mixed, BarcodeFormat::UPCA, "012345678905", upca);
  }
}); }
