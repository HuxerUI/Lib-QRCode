#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace huxerui::qrcode {

enum class BarcodeFormat {
  QRCode, Code128, Code39, Code93, EAN8, EAN13, UPCA, UPCE, ITF, Codabar,
  DataMatrix, Aztec, PDF417,
  MicroQRCode, RMQRCode, MaxiCode, DataBar, DataBarExpanded, DataBarLimited, DXFilmEdge,
};

[[nodiscard]] std::string_view FormatName(BarcodeFormat format);
[[nodiscard]] bool CanEncode(BarcodeFormat format) noexcept;

enum class ErrorCorrection { Low, Medium, Quartile, High };

struct EncodeOptions {
  BarcodeFormat format = BarcodeFormat::QRCode;
  ErrorCorrection qr_error_correction = ErrorCorrection::Medium;
  /// Modules on every side; -1 selects a format-specific default. Range: -1..256.
  int quiet_zone = -1;
  /// Height of linear bars in modules (1..1024); ignored for matrix formats.
  int bar_height = 64;
};

enum class PixelFormat { Gray8, RGB8, BGR8, RGBA8, BGRA8 };

/// Borrowed, top-down pixels. Storage must stay alive throughout synchronous Decode/DecodeAll.
/// Alpha is ignored; composite transparent images onto a light background before decoding.
struct ImageView {
  std::span<const std::uint8_t> pixels;
  int width = 0;
  int height = 0;
  PixelFormat format = PixelFormat::Gray8;
  /// Bytes between rows; zero means tightly packed. Negative strides are unsupported.
  int row_stride = 0;
};

struct GrayImage {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> pixels;
  [[nodiscard]] ImageView View() const noexcept { return {pixels, width, height}; }
};

struct SvgOptions {
  int scale = 4;
  /// Opaque RGB colors in 0xRRGGBB order.
  std::uint32_t foreground = 0x000000;
  std::uint32_t background = 0xFFFFFF;
};

/// Immutable, cheaply copied symbol. Modules include the quiet zone; 1 is dark and 0 is light.
class Symbol final {
public:
  [[nodiscard]] int Width() const noexcept;
  [[nodiscard]] int Height() const noexcept;
  [[nodiscard]] BarcodeFormat Format() const noexcept;
  [[nodiscard]] std::span<const std::uint8_t> Modules() const noexcept;
  /// Throws std::out_of_range for coordinates outside the symbol.
  [[nodiscard]] bool IsDark(int x, int y) const;
  /// Integer nearest-neighbor scaling. Scale must be positive; output is limited to 64 Mi pixels.
  [[nodiscard]] GrayImage Rasterize(int scale = 4) const;
  [[nodiscard]] std::string ToSvg(SvgOptions options = {}) const;

private:
  struct Data;
  std::shared_ptr<const Data> data_;
  explicit Symbol(BarcodeFormat format, int width, int height, std::vector<std::uint8_t> modules);
  friend Symbol Encode(std::string_view text, EncodeOptions options);
};

/// UTF-8 text, including embedded NULs. Rejects empty/invalid UTF-8 and inputs over 8192 bytes.
/// Format-specific capacity and alphabet restrictions also apply. Throws std::invalid_argument.
[[nodiscard]] Symbol Encode(std::string_view text, EncodeOptions options = {});

struct DecodeOptions {
  /// Empty means every supported reader format.
  std::vector<BarcodeFormat> formats;
  bool try_harder = true;
  bool try_rotate = true;
  bool try_invert = true;
  /// Use only for an isolated, perfectly aligned symbol; leave false for camera frames.
  bool pure = false;
  /// Maximum returned symbols, 1..255.
  int max_symbols = 32;
};

struct ImagePoint {
  int x = 0;
  int y = 0;
  bool operator==(const ImagePoint&) const = default;
};

struct DecodeResult {
  BarcodeFormat format = BarcodeFormat::QRCode;
  std::string text;
  std::vector<std::uint8_t> bytes;
  /// Top-left, top-right, bottom-right, bottom-left, in input pixel coordinates.
  std::array<ImagePoint, 4> position;
  int orientation = 0;
  bool mirrored = false;
  bool inverted = false;
  std::string error_correction;
  std::string symbology_identifier;
};

/// Returns only successfully decoded symbols. No match is an empty vector/nullopt, not an error.
/// Invalid image layouts/options throw std::invalid_argument. Maximum input: 64 Mi pixels.
[[nodiscard]] std::vector<DecodeResult> DecodeAll(ImageView image, DecodeOptions options = {});
[[nodiscard]] std::optional<DecodeResult> Decode(ImageView image, DecodeOptions options = {});

} // namespace huxerui::qrcode
