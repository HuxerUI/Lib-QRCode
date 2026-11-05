#pragma once

/// @file
/// Stateless QR code and barcode Views for HuxerUI. No installation hook is required.
/// Supply bounded layout through a parent or Frame, and accessibility text through Semantics.
/// Text overloads encode synchronously; prepared symbols retain shared immutable module storage.

#include <huxerui/qrcode/codec.h>
#include <huxerui/color.h>
#include <huxerui/view.h>

namespace huxerui::qrcode {

/// Paint-only configuration. Defaults preserve black-on-white contrast in either application theme.
/// Every channel must be finite and in [0, 1]. Custom colors remain the caller's contrast responsibility.
struct BarcodeStyle {
  Color foreground = Color::Black();
  Color background = Color::White();
  bool operator==(const BarcodeStyle&) const = default;
};

/// QR-specific encoding configuration. Layout and presentation are configured independently.
struct QRCodeOptions {
  /// Requested QR correction level. Medium is the default.
  ErrorCorrection error_correction = ErrorCorrection::Medium;
  /// Quiet modules on every side; -1 means four. Valid range: -1..256.
  int quiet_zone = -1;
  bool operator==(const QRCodeOptions&) const = default;
};

/// Displays a prepared symbol using centered, aspect-preserving native drawing.
/// No intrinsic layout size is imposed. Supply positive bounds through a parent or Frame modifier.
/// Quiet zones are part of the symbol. Integer module sizes are used when space permits;
/// small bounds and fractional display scaling can reduce scan reliability.
/// Recomposition copies the shared symbol handle without encoding it again.
/// The default image label is the format name; the payload is never exposed as accessible text.
/// Override the label with Semantics, which accepts literals and generated string resources.
/// @throws std::invalid_argument If a style channel is nonfinite or outside [0, 1].
/// @code
/// Barcode(symbol).With(Frame{.width = 320.0F, .height = 120.0F},
///                      Semantics{.label = "Inventory barcode"});
/// @endcode
[[nodiscard]] View Barcode(Symbol symbol, BarcodeStyle style = {});

/// Encodes text synchronously and displays it with Barcode(Symbol, style).
/// Encoding options are explicit; their format is used without substitution.
/// For repeated composition, Encode once and retain a Symbol in application state.
/// @throws std::invalid_argument For invalid configuration, text, or format capacity overflow.
[[nodiscard]] View Barcode(std::string_view text, EncodeOptions options, BarcodeStyle style = {});

/// Displays a prepared QR symbol with the same layout, colors, and semantics as Barcode.
/// @throws std::invalid_argument If symbol.Format() is not BarcodeFormat::QRCode, or style is invalid.
[[nodiscard]] View QRCode(Symbol symbol, BarcodeStyle style = {});

/// Encodes UTF-8 text as QR Code, then displays it with Barcode(Symbol, style).
/// Supports the same payload restrictions as Encode. Invoke during composition; no global cache is retained.
/// @param options QR correction and quiet-zone request, independent of layout and painting.
/// @throws std::invalid_argument For invalid options, style, UTF-8, or QR capacity overflow.
[[nodiscard]] View QRCode(std::string_view text, QRCodeOptions options = {}, BarcodeStyle style = {});

} // namespace huxerui::qrcode
