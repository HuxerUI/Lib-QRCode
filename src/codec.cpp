#include <huxerui/qrcode/codec.h>

#include <Barcode.h>
#include <BitMatrix.h>
#include <CharacterSet.h>
#include <ImageView.h>
#include <MultiFormatWriter.h>
#include <ReadBarcode.h>
#include <ReaderOptions.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace huxerui::qrcode {
namespace {

constexpr std::size_t max_pixels = 64U * 1024U * 1024U;
constexpr std::array formats{
  ZXing::BarcodeFormat::QRCode, ZXing::BarcodeFormat::Code128, ZXing::BarcodeFormat::Code39,
  ZXing::BarcodeFormat::Code93, ZXing::BarcodeFormat::EAN8, ZXing::BarcodeFormat::EAN13,
  ZXing::BarcodeFormat::UPCA, ZXing::BarcodeFormat::UPCE, ZXing::BarcodeFormat::ITF,
  ZXing::BarcodeFormat::Codabar, ZXing::BarcodeFormat::DataMatrix, ZXing::BarcodeFormat::Aztec,
  ZXing::BarcodeFormat::PDF417, ZXing::BarcodeFormat::MicroQRCode, ZXing::BarcodeFormat::RMQRCode,
  ZXing::BarcodeFormat::MaxiCode, ZXing::BarcodeFormat::DataBar, ZXing::BarcodeFormat::DataBarExpanded,
  ZXing::BarcodeFormat::DataBarLimited, ZXing::BarcodeFormat::DXFilmEdge,
};
constexpr std::array<std::string_view, formats.size()> names{
  "QR Code", "Code 128", "Code 39", "Code 93", "EAN-8", "EAN-13", "UPC-A", "UPC-E", "ITF", "Codabar",
  "Data Matrix", "Aztec", "PDF417", "Micro QR Code", "rMQR Code", "MaxiCode", "GS1 DataBar",
  "GS1 DataBar Expanded", "GS1 DataBar Limited", "DX Film Edge",
};

[[noreturn]] void Invalid(std::string_view message) {
  throw std::invalid_argument("HuxerUI QRCode: " + std::string(message));
}

ZXing::BarcodeFormat ToBackend(BarcodeFormat format) {
  const auto index = static_cast<std::size_t>(format);
  if(index >= formats.size()) Invalid("invalid barcode format");
  return formats[index];
}

BarcodeFormat FromBackend(ZXing::BarcodeFormat format) {
  const auto found = std::find(formats.begin(), formats.end(), format);
  if(found == formats.end()) throw std::logic_error("HuxerUI QRCode: unrecognized backend format");
  return static_cast<BarcodeFormat>(found - formats.begin());
}

bool IsLinear(BarcodeFormat format) {
  return format >= BarcodeFormat::Code128 && format <= BarcodeFormat::Codabar;
}

void ValidateUtf8(std::string_view text) {
  std::uint32_t value = 0, minimum = 0;
  int remaining = 0;
  for(unsigned char byte : text) {
    if(remaining == 0) {
      if(byte <= 0x7F) continue;
      if(byte >= 0xC2 && byte <= 0xDF) { remaining = 1; value = byte & 0x1FU; minimum = 0x80; }
      else if(byte >= 0xE0 && byte <= 0xEF) { remaining = 2; value = byte & 0x0FU; minimum = 0x800; }
      else if(byte >= 0xF0 && byte <= 0xF4) { remaining = 3; value = byte & 0x07U; minimum = 0x10000; }
      else Invalid("text must be valid UTF-8");
    } else {
      if((byte & 0xC0U) != 0x80U) Invalid("text must be valid UTF-8");
      value = (value << 6U) | (byte & 0x3FU);
      if(--remaining == 0 && (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)))
        Invalid("text must be valid UTF-8");
    }
  }
  if(remaining) Invalid("text must be valid UTF-8");
}

std::size_t CheckedPixels(int width, int height) {
  if(width <= 0 || height <= 0 || static_cast<std::size_t>(width) > max_pixels / static_cast<std::size_t>(height))
    Invalid("image dimensions must be positive and at most 64 Mi pixels");
  return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::pair<int, int> ScaledSize(int width, int height, int scale) {
  if(scale <= 0 || width > std::numeric_limits<int>::max() / scale || height > std::numeric_limits<int>::max() / scale)
    Invalid("invalid raster scale");
  CheckedPixels(width * scale, height * scale);
  return {width * scale, height * scale};
}

std::string HexColor(std::uint32_t color) {
  if(color > 0xFFFFFF) Invalid("SVG colors must be RGB values in 0xRRGGBB order");
  std::string result(7, '#');
  constexpr char hex[] = "0123456789abcdef";
  for(int i = 6; i > 0; --i, color >>= 4U) result[static_cast<std::size_t>(i)] = hex[color & 15U];
  return result;
}

ZXing::ImageView BackendImage(ImageView image) {
  CheckedPixels(image.width, image.height);
  ZXing::ImageFormat format;
  int channels;
  switch(image.format) {
    case PixelFormat::Gray8: format = ZXing::ImageFormat::Lum; channels = 1; break;
    case PixelFormat::RGB8: format = ZXing::ImageFormat::RGB; channels = 3; break;
    case PixelFormat::BGR8: format = ZXing::ImageFormat::BGR; channels = 3; break;
    case PixelFormat::RGBA8: format = ZXing::ImageFormat::RGBA; channels = 4; break;
    case PixelFormat::BGRA8: format = ZXing::ImageFormat::BGRA; channels = 4; break;
    default: Invalid("invalid pixel format");
  }
  if(image.width > std::numeric_limits<int>::max() / channels) Invalid("image row is too wide");
  const int row_bytes = image.width * channels;
  const int stride = image.row_stride == 0 ? row_bytes : image.row_stride;
  if(stride < row_bytes) Invalid("row stride is smaller than a pixel row");
  // The backend uses signed int offsets, including during downscaling and rotation.
  const auto storage_size = static_cast<std::uint64_t>(stride) * static_cast<unsigned>(image.height);
  if(storage_size > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) Invalid("image stride is too large");
  const auto required = storage_size - static_cast<unsigned>(stride) + static_cast<unsigned>(row_bytes);
  if(image.pixels.data() == nullptr || image.pixels.size() < required) Invalid("image buffer is too small");
  return {image.pixels.data(), image.width, image.height, format, stride};
}

} // namespace

struct Symbol::Data {
  BarcodeFormat format;
  int width;
  int height;
  std::vector<std::uint8_t> modules;
};

Symbol::Symbol(BarcodeFormat format, int width, int height, std::vector<std::uint8_t> modules)
    : data_(std::make_shared<const Data>(Data{format, width, height, std::move(modules)})) {}
int Symbol::Width() const noexcept { return data_->width; }
int Symbol::Height() const noexcept { return data_->height; }
BarcodeFormat Symbol::Format() const noexcept { return data_->format; }
std::span<const std::uint8_t> Symbol::Modules() const noexcept { return data_->modules; }

bool Symbol::IsDark(int x, int y) const {
  if(x < 0 || y < 0 || x >= Width() || y >= Height()) throw std::out_of_range("HuxerUI QRCode: module out of range");
  return data_->modules[static_cast<std::size_t>(y) * Width() + x] != 0;
}

std::string_view FormatName(BarcodeFormat format) {
  ToBackend(format);
  return names[static_cast<std::size_t>(format)];
}

bool CanEncode(BarcodeFormat format) noexcept {
  return static_cast<std::size_t>(format) <= static_cast<std::size_t>(BarcodeFormat::PDF417);
}

Symbol Encode(std::string_view text, EncodeOptions options) {
  const auto format = ToBackend(options.format);
  if(!CanEncode(options.format)) Invalid("the selected format is decode-only");
  if(text.empty() || text.size() > 8192) Invalid("text length must be 1..8192 UTF-8 bytes");
  ValidateUtf8(text);
  if(options.quiet_zone < -1 || options.quiet_zone > 256) Invalid("quiet zone must be -1..256 modules");
  if(options.bar_height < 1 || options.bar_height > 1024) Invalid("bar height must be 1..1024 modules");
  const auto ecc = static_cast<unsigned>(options.qr_error_correction);
  if(ecc > 3) Invalid("invalid QR error correction level");
  int margin = options.quiet_zone;
  if(margin == -1) margin = options.format == BarcodeFormat::QRCode ? 4 : IsLinear(options.format) ? 10 : 2;
  ZXing::BitMatrix matrix;
  try {
    auto writer = ZXing::MultiFormatWriter(format).setMargin(0);
    writer.setEncoding(ZXing::CharacterSet::UTF8);
    if(options.format == BarcodeFormat::QRCode) writer.setEccLevel(static_cast<int>(ecc) * 2 + 1);
    matrix = writer.encode(std::string(text), 0, IsLinear(options.format) ? options.bar_height : 0);
  } catch(const std::invalid_argument& error) {
    Invalid(error.what());
  } catch(const std::length_error& error) {
    Invalid(error.what());
  }
  const int width = matrix.width() + margin * 2;
  const int height = matrix.height() + margin * 2;
  std::vector<std::uint8_t> modules(CheckedPixels(width, height), 0);
  for(int y = 0; y < matrix.height(); ++y)
    for(int x = 0; x < matrix.width(); ++x)
      modules[static_cast<std::size_t>(y + margin) * width + x + margin] = matrix.get(x, y) ? 1 : 0;
  return Symbol(options.format, width, height, std::move(modules));
}

GrayImage Symbol::Rasterize(int scale) const {
  const auto [width, height] = ScaledSize(Width(), Height(), scale);
  GrayImage image{width, height, std::vector<std::uint8_t>(CheckedPixels(width, height))};
  for(int y = 0; y < height; ++y)
    for(int x = 0; x < width; ++x)
      image.pixels[static_cast<std::size_t>(y) * width + x] = IsDark(x / scale, y / scale) ? 0 : 255;
  return image;
}

std::string Symbol::ToSvg(SvgOptions options) const {
  const auto [width, height] = ScaledSize(Width(), Height(), options.scale);
  const auto foreground = HexColor(options.foreground);
  const auto background = HexColor(options.background);
  std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + std::to_string(width) +
      "\" height=\"" + std::to_string(height) + "\" viewBox=\"0 0 " + std::to_string(Width()) + " " +
      std::to_string(Height()) + "\" shape-rendering=\"crispEdges\"><path fill=\"" + background +
      "\" d=\"M0 0h" + std::to_string(Width()) + "v" + std::to_string(Height()) + "H0z\"/><path fill=\"" +
      foreground + "\" d=\"";
  for(int y = 0; y < Height(); ++y) {
    for(int x = 0; x < Width();) {
      if(!IsDark(x, y)) { ++x; continue; }
      const int start = x++;
      while(x < Width() && IsDark(x, y)) ++x;
      svg += "M" + std::to_string(start) + " " + std::to_string(y) + "h" + std::to_string(x - start) + "v1H" +
          std::to_string(start) + "z";
    }
  }
  return svg + "\"/></svg>";
}

std::vector<DecodeResult> DecodeAll(ImageView image, DecodeOptions options) {
  const auto input = BackendImage(image);
  if(options.max_symbols < 1 || options.max_symbols > 255) Invalid("max_symbols must be 1..255");
  ZXing::BarcodeFormats formats_filter;
  for(auto format : options.formats) formats_filter |= ToBackend(format);
  ZXing::ReaderOptions hints;
  hints.setFormats(formats_filter).setTryHarder(options.try_harder).setTryRotate(options.try_rotate)
      .setTryInvert(options.try_invert).setIsPure(options.pure).setTextMode(ZXing::TextMode::Plain)
      .setMaxNumberOfSymbols(static_cast<std::uint8_t>(options.max_symbols));
  std::vector<DecodeResult> results;
  for(const auto& decoded : ZXing::ReadBarcodes(input, hints)) {
    if(!decoded.isValid()) continue;
    DecodeResult result;
    result.format = FromBackend(decoded.format());
    result.text = decoded.text();
    result.bytes.assign(decoded.bytes().begin(), decoded.bytes().end());
    for(std::size_t i = 0; i < result.position.size(); ++i)
      result.position[i] = {decoded.position()[i].x, decoded.position()[i].y};
    result.orientation = decoded.orientation();
    result.mirrored = decoded.isMirrored();
    result.inverted = decoded.isInverted();
    result.error_correction = decoded.ecLevel();
    result.symbology_identifier = decoded.symbologyIdentifier();
    results.push_back(std::move(result));
  }
  return results;
}

std::optional<DecodeResult> Decode(ImageView image, DecodeOptions options) {
  if(options.max_symbols < 1 || options.max_symbols > 255) Invalid("max_symbols must be 1..255");
  options.max_symbols = 1;
  auto results = DecodeAll(image, std::move(options));
  if(results.empty()) return std::nullopt;
  return std::move(results.front());
}

} // namespace huxerui::qrcode
