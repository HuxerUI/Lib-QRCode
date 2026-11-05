# Lib-QRCode

[简体中文](README.zh-CN.md)

Native QR codes and barcodes for [HuxerUI](https://github.com/HuxerUI/HuxerUI): generate symbols, render declarative views, export SVG or grayscale pixels, and decode image buffers.

- C++20; namespace `huxerui::qrcode`, CMake target `HuxerUI::QRCode` (`huxerui_qrcode`).
- Generate QR Code, Code 128/39/93, EAN-8/13, UPC-A/E, ITF, Codabar, Data Matrix, Aztec, and PDF417.
- Read those formats plus Micro QR, rMQR, MaxiCode, GS1 DataBar/Expanded/Limited, and DX Film Edge.
- UTF-8 text, four QR error correction levels, configurable quiet zones, immutable shared symbols.
- Native `Canvas` rendering with preserved aspect ratio, integer module sizing when space permits, and accessibility labels.
- Multi-symbol decoding from Gray8, RGB, BGR, RGBA, and BGRA buffers, including padded rows.
- Vendored ZXing-C++ 2.3.0 with a Windows PDF417 accumulator fix; no build-time downloads for the codec.

The shared codec and views use no platform-specific APIs. Windows builds and tests are verified locally; CI covers standalone codecs on Linux, macOS, and Windows, plus native view tests on Windows. Other HuxerUI backends require their normal SDK/toolchain and have not been verified here.

## Add to a HuxerUI app

```cmake
# After huxerui_add_app(your_app ...):
huxerui_use_library(your_app
    TARGET HuxerUI::QRCode
    PATH "/path/to/Lib-QRCode")
```

```cpp
#include <huxerui/huxerui.h>
#include <huxerui/qrcode.h>

using namespace huxerui;
using namespace huxerui::qrcode;

View Ticket() {
  return Column {
    QRCode("https://github.com/HuxerUI", {
      .error_correction = ErrorCorrection::High,
    }).With(Frame{.width = 240.0F, .height = 240.0F}),
    Barcode("HUXERUI-2026", {.format = BarcodeFormat::Code128})
        .With(Frame{.width = 480.0F, .height = 120.0F}, Semantics{.label = "Inventory barcode"}),
  }.With(Spacing(16.0F));
}
```

`QRCode(text, QRCodeOptions)` always encodes QR Code; the text overload of `Barcode` takes explicit `EncodeOptions` and uses their format unchanged. Both also accept prepared `Symbol` values; `QRCode(symbol)` validates that the format is QR Code. All view overloads use the same `BarcodeStyle{}` defaults and impose no intrinsic frame. Supply bounded layout through a parent or `.With(Frame{...})`.

White backgrounds and black modules are intentional in both light and dark themes. Custom colors are supported but callers must preserve enough contrast. Keep quiet zones unobstructed and provide at least one device pixel per module; very small bounds, fractional display scaling, clipping, or later transforms can reduce scan reliability. This component does not draw logos over symbols or round their modules.

Payloads are not automatically exposed as accessible text. The default image label is the symbol's format name. Override it with `.With(Semantics{.label = "Ticket code"})`; the SDK accepts literal text, `StringVariant`, and generated localized string resources. `BarcodeStyle` controls only foreground/background colors.

## Prepare once, reuse and export

The core header has no HuxerUI dependency:

```cpp
#include <huxerui/qrcode/codec.h>
#include <fstream>

using namespace huxerui::qrcode;

auto symbol = Encode("二维码 / 😀", {
  .qr_error_correction = ErrorCorrection::Quartile,
});
auto pixels = symbol.Rasterize(8); // 8 pixels per module, black=0, white=255
auto svg = symbol.ToSvg({.scale = 8});
std::ofstream("code.svg", std::ios::binary) << svg;
```

On the UI thread, `QRCode(symbol).With(Frame{.width = 280.0F, .height = 280.0F}, Semantics{.label = "Ticket code"})` renders a prepared QR symbol; `Barcode(symbol)` accepts any writable symbol. Copies share immutable module storage and compare equal by allocation identity. Separate `Encode` calls produce distinct identities even for equal payloads. A worker can encode once and pass the result to application-owned state. Text view overloads encode immediately on every call; they do not maintain a global cache.

`Width()`, `Height()`, `Modules()`, and `IsDark(x, y)` include the quiet zone. `Modules()` is a row-major byte span containing 0/1 values and stays valid while a `Symbol` retains its allocation. `GrayImage::View()` borrows the image's pixel storage. `Rasterize` preserves exact module boundaries using positive integer scaling. SVG contains only geometric paths and opaque RGB colors; the payload is not embedded as text or metadata. Both exports default to black on white.

## Decode pixels or camera frames

```cpp
auto result = Decode(pixels.View());
if (result) {
  // result->text is UTF-8; result->bytes preserves original encoded bytes.
}

// Borrow an RGB(A) frame from your image loader or camera adapter:
auto results = DecodeAll({rgba_bytes, width, height, PixelFormat::RGBA8, row_stride}, {
  .formats = {BarcodeFormat::QRCode, BarcodeFormat::Code128},
  .max_symbols = 8,
});
```

`rgba_bytes` is a `std::span<const std::uint8_t>` or a compatible contiguous container. Buffers are borrowed only during the synchronous call. Results own text, raw bytes, format, four input-pixel corner positions, orientation, mirror/inversion flags, error correction level, and symbology identifier. Corner order is top-left, top-right, bottom-right, bottom-left in symbol orientation; linear formats can have degenerate quadrilaterals.

No match returns `std::nullopt` or an empty vector. Only valid results are returned. An empty format filter enables all readers. `try_harder`, `try_rotate`, and `try_invert` default to true; `pure` defaults to false and should only be enabled for a single isolated, perfectly aligned symbol. `Decode` returns one match, with no guaranteed ordering; `DecodeAll` returns up to `max_symbols` (default 32, maximum 255).

Pixels must be top-down with positive dimensions and a nonnegative row stride; zero stride means tightly packed. Alpha is ignored, so composite transparent images before decoding. YUV camera adapters can pass a contiguous luminance plane as Gray8 when its pixel stride is one. Packed/interleaved YUV, negative row strides, image-file loading, camera permissions, capture sessions, background scheduling, and repeated-scan debouncing belong to the application or its camera/image library. Copy or retain frames for worker decoding; the component does not retain them.

## Validation and format details

- `Encode` accepts 1..8192 valid UTF-8 bytes, including embedded NULs. Each format's actual capacity/alphabet can be smaller. This is a text encoder; binary decode results remain available through `bytes`.
- `EncodeOptions::quiet_zone` accepts -1..256 modules. -1 means 4 for QR, 10 for linear formats, and 2 for other matrix formats, on every side. Explicit zero is permitted for controlled composition; add suitable external margins before scanning.
- Linear `bar_height` accepts 1..1024 modules and defaults to 64. QR correction defaults to Medium; the QR-specific option has no effect on other formats.
- `QRCodeOptions` contains only `error_correction` and `quiet_zone`. `BarcodeStyle`, `QRCodeOptions`, `EncodeOptions`, `DecodeOptions`, and `SvgOptions` support value equality for application-owned configuration.
- EAN/UPC digits and checksums are validated by the backend. Payloads omitting a check digit may have it added by the backend; decoding returns the full canonical value. Codabar includes start/stop letters. Text restrictions and extended Code 39/93 interpretation follow ZXing.
- Invalid options, UTF-8, layouts, unsupported encoding formats, or capacity overflow throw `std::invalid_argument`. Out-of-range `IsDark` coordinates throw `std::out_of_range`. Allocation and unexpected internal errors retain their exception categories.
- Raster/scan dimensions are limited to 64 Mi pixels. Decoding also checks span length and signed backend row-offset limits. Native view bounds follow the SDK's layout validation; an empty drawing area produces no commands.
- SVG supports opaque `0xRRGGBB` foreground/background overrides. Color customization does not guarantee contrast or scan quality.

## Build and test

Requires CMake 3.20+ and C++20. The native component uses HuxerUI source or SDK; the verified framework revision is `cadb1696cfa7b87d9d2d316b3ecba07b85a46962`.

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_QRCODE_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_QRCODE_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run `example_qrcode` to select any writable format, edit its payload, choose QR correction, load valid samples, change themes, decode the generated preview, and export SVG through the system save dialog. Like the other Lib projects, the demo lives in [`examples/preview`](examples/preview/README.md) and can also be configured independently:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

For the standalone codec without HuxerUI:

```sh
cmake -S . -B build-core -DHUXERUI_QRCODE_BUILD_VIEW=OFF \
  -DHUXERUI_QRCODE_BUILD_TESTS=ON
cmake --build build-core --config Release --parallel
ctest --test-dir build-core -C Release --output-on-failure
```

Both modes expose `HuxerUI::QRCode`. The UI header requires the native build; standalone consumers include only `<huxerui/qrcode/codec.h>`. HuxerUI library-graph and project-plan discovery do not compile or configure ZXing. The pinned backend owns the `ZXing`/`ZXing::ZXing` targets; avoid combining another independently configured ZXing copy in the same CMake build. There is no standalone install/export package in this version; consume it through `add_subdirectory` or `huxerui_use_library`.

The public API follows the [interface conventions](docs/interface-conventions.md) reviewed from FinalScave's libraries.

## License

MIT for Lib-QRCode. Vendored ZXing-C++ uses Apache-2.0 and includes BSD-3-Clause libzueci. See [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md).
