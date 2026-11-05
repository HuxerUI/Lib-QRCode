#pragma once

#include <huxerui/qrcode/codec.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::qrcode {

struct BarcodeStyle {
  Size size{240.0F, 240.0F};
  Color foreground = Color::Black();
  Color background = Color::White();
  /// Accessible name. Payload text is never exposed to accessibility automatically.
  std::string label = "Barcode";
};

/// Native vector rendering, centered with a preserved aspect ratio and quiet zone.
/// Uses integer module sizes when space permits; very small bounds may be unscannable.
/// The prepared-symbol overload shares immutable data across recompositions.
[[nodiscard]] View Barcode(Symbol symbol, BarcodeStyle style = {});
[[nodiscard]] View Barcode(std::string_view text, EncodeOptions options = {.format = BarcodeFormat::Code128},
                           BarcodeStyle style = {.size = {320.0F, 120.0F}});
/// Encodes synchronously. options.format must be QRCode.
[[nodiscard]] View QRCode(std::string_view text, EncodeOptions options = {},
                          BarcodeStyle style = {.label = "QR Code"});

} // namespace huxerui::qrcode
