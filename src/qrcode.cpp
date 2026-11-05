#include <huxerui/qrcode.h>
#include <huxerui/modifier.h>
#include <huxerui/semantics.h>

#include "paint_symbol.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::qrcode {
namespace {

void ValidateStyle(const BarcodeStyle& style) {
  if(!std::isfinite(style.size.width) || !std::isfinite(style.size.height) ||
      style.size.width <= 0 || style.size.height <= 0 || style.size.width > 16384 || style.size.height > 16384)
    throw std::invalid_argument("HuxerUI QRCode: view dimensions must be finite and in (0, 16384]");
  for(auto color : {style.foreground, style.background}) {
    for(float channel : {color.red, color.green, color.blue, color.alpha})
      if(!std::isfinite(channel) || channel < 0 || channel > 1)
        throw std::invalid_argument("HuxerUI QRCode: color channels must be finite and in [0, 1]");
  }
}

} // namespace

void detail::PaintSymbol(PaintContext& paint, Size size, const Symbol& symbol, Color foreground, Color background) {
  if(!std::isfinite(size.width) || !std::isfinite(size.height) || size.width <= 0 || size.height <= 0) return;
  paint.DrawRect({0, 0, size.width, size.height}, background);
  float scale = std::min(size.width / static_cast<float>(symbol.Width()),
                         size.height / static_cast<float>(symbol.Height()));
  if(scale >= 1) scale = std::floor(scale);
  const float left = std::floor((size.width - static_cast<float>(symbol.Width()) * scale) / 2);
  const float top = std::floor((size.height - static_cast<float>(symbol.Height()) * scale) / 2);
  const auto modules = symbol.Modules();
  for(int y = 0; y < symbol.Height(); ++y) {
    for(int x = 0; x < symbol.Width();) {
      const auto row = static_cast<std::size_t>(y) * symbol.Width();
      if(!modules[row + x]) { ++x; continue; }
      const int start = x++;
      while(x < symbol.Width() && modules[row + x]) ++x;
      paint.DrawRect({left + static_cast<float>(start) * scale, top + static_cast<float>(y) * scale,
                      static_cast<float>(x - start) * scale, scale}, foreground);
    }
  }
}

View Barcode(Symbol symbol, BarcodeStyle style) {
  ValidateStyle(style);
  return Canvas([symbol = std::move(symbol), foreground = style.foreground, background = style.background]
                (PaintContext& paint, Size size) {
    detail::PaintSymbol(paint, size, symbol, foreground, background);
  }).With(Frame{.width = style.size.width, .height = style.size.height},
           Semantics{.role = SemanticRole::Image, .label = std::move(style.label)});
}

View Barcode(std::string_view text, EncodeOptions options, BarcodeStyle style) {
  ValidateStyle(style);
  return Barcode(Encode(text, options), std::move(style));
}

View QRCode(std::string_view text, EncodeOptions options, BarcodeStyle style) {
  if(options.format != BarcodeFormat::QRCode)
    throw std::invalid_argument("HuxerUI QRCode: QRCode requires the QRCode format; use Barcode for other formats");
  return Barcode(text, options, std::move(style));
}

} // namespace huxerui::qrcode
