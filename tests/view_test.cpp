#include <huxerui/qrcode.h>
#include <huxerui/huxerui.h>

#include "paint_symbol.h"
#include "test.h"

#include <cmath>
#include <limits>
#ifdef QRCODE_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
#endif

using namespace huxerui;
using namespace huxerui::qrcode;

void CheckPaint(const Symbol& symbol, int width, int height) {
  PaintSequence sequence;
  PaintContext paint(sequence, {0, 0, static_cast<float>(width), static_cast<float>(height)});
  qrcode::detail::PaintSymbol(paint, {static_cast<float>(width), static_cast<float>(height)},
                             symbol, Color::Black(), Color::White());
  paint.Finish();
  CHECK(sequence.Commands().size() > 1);
  GrayImage image{width, height, std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, 127)};
  for(const auto& command : sequence.Commands()) {
    const auto& rect = std::get<DrawRectCommand>(command);
    CHECK(rect.rect.x >= 0 && rect.rect.y >= 0);
    CHECK(rect.rect.x + rect.rect.width <= width);
    CHECK(rect.rect.y + rect.rect.height <= height);
    CHECK(rect.rect.width == std::floor(rect.rect.width));
    CHECK(rect.rect.height == std::floor(rect.rect.height));
    const auto color = std::get<Color>(rect.brush.Get());
    for(int y = static_cast<int>(rect.rect.y); y < rect.rect.y + rect.rect.height; ++y)
      for(int x = static_cast<int>(rect.rect.x); x < rect.rect.x + rect.rect.width; ++x)
        image.pixels[static_cast<std::size_t>(y) * width + x] = color == Color::Black() ? 0 : 255;
  }
  const auto result = Decode(image.View(), {.formats = {symbol.Format()}});
  CHECK(result.has_value());
  CHECK(result->text == "HUXERUI-2026");
}

#ifdef QRCODE_HAS_UI_TESTING
View snapshot_declaration;

huxerui::testing::UiSnapshot RenderDeclaration(View declaration) {
  snapshot_declaration = std::move(declaration);
  Application app([] { return snapshot_declaration; }, {.show_debug_overlay = false});
  huxerui::testing::UiTest ui(app);
  return ui.CaptureSnapshot();
}

void CheckViewContracts() {
  const Frame frame{.width = 240.0F, .height = 240.0F};
  CHECK(RenderDeclaration(QRCode("default").With(frame)) ==
        RenderDeclaration(QRCode(Encode("default")).With(frame)));
  const BarcodeStyle style{.foreground = Color::Rgb(20, 40, 60), .background = Color::Rgb(245, 245, 255)};
  const auto prepared = Encode("configured", {.qr_error_correction = ErrorCorrection::High, .quiet_zone = 8});
  CHECK(RenderDeclaration(QRCode("configured", {.error_correction = ErrorCorrection::High, .quiet_zone = 8}, style)
                              .With(frame)) == RenderDeclaration(Barcode(prepared, style).With(frame)));
  CHECK(RenderDeclaration(Barcode("HUXERUI", {.format = BarcodeFormat::Code128}, style).With(frame)) ==
        RenderDeclaration(Barcode(Encode("HUXERUI", {.format = BarcodeFormat::Code128}), style).With(frame)));
}

void CheckLayout() {
  using namespace huxerui::testing;
  Application app([]() -> View {
    return Column {
      Barcode(Encode("prepared"))
          .With(Frame{.width = 160.0F, .height = 100.0F}).Key("prepared"),
      Barcode("123456", {.format = BarcodeFormat::Code128})
          .With(Frame{.width = 160.0F, .height = 100.0F}).Key("encoded"),
      QRCode("qr").With(Frame{.width = 160.0F, .height = 100.0F}).Key("text-qr"),
      QRCode(Encode("qr")).With(Frame{.width = 160.0F, .height = 100.0F}).Key("prepared-qr"),
      Row { QRCode("parent bounds").With(Grow{}).Key("parent-bounds") }
          .With(Frame{.width = 120.0F, .height = 80.0F}, CrossAlign(CrossAxisAlignment::Stretch)),
    };
  }, {.show_debug_overlay = false});
  UiTest ui(app);
  for(const char* key : {"prepared", "encoded", "text-qr", "prepared-qr"})
    CHECK(ui.Find(UiSelector::Key(key)).One().size == (Size{160, 100}));
  CHECK(ui.Find(UiSelector::Key("parent-bounds")).One().size == (Size{120, 80}));
  CHECK(ui.FindSemantics(UiSemanticSelector::Label("Code 128")).Exists());
  CHECK(ui.FindSemantics(UiSemanticSelector::Label("QR Code")).Exists());
}
#endif

int main() { return Run([] {
  const auto symbol = Encode("HUXERUI-2026");
  CheckPaint(symbol, 241, 320);
  CheckPaint(symbol, 400, 201);
  CheckPaint(Encode("HUXERUI-2026", {.format = BarcodeFormat::Code128}), 640, 160);
  QRCODE_TEST_DISCARD(QRCode("Hello"));
  QRCODE_TEST_DISCARD(Barcode("123456", {.format = BarcodeFormat::Code128}));
  QRCODE_TEST_DISCARD(Barcode(symbol));
  QRCODE_TEST_DISCARD(QRCode(symbol));
  Throws<std::invalid_argument>([] { QRCODE_TEST_DISCARD(QRCode(Encode("x", {.format = BarcodeFormat::Code128}))); });
  Throws<std::invalid_argument>([] { QRCODE_TEST_DISCARD(QRCode("x", {.error_correction = static_cast<ErrorCorrection>(99)})); });
  Throws<std::invalid_argument>([] { QRCODE_TEST_DISCARD(QRCode("x", {.quiet_zone = -2})); });
  Throws<std::invalid_argument>([&] { QRCODE_TEST_DISCARD(Barcode(symbol, {.foreground = {2, 0, 0, 1}})); });
  Throws<std::invalid_argument>([&] {
    QRCODE_TEST_DISCARD(Barcode(symbol, {.background = {0, 0, 0, std::numeric_limits<float>::infinity()}}));
  });

#ifdef QRCODE_HAS_UI_TESTING
  CheckViewContracts();
  CheckLayout();
  using namespace huxerui::testing;
  Application app([]() -> View {
    auto changed = UseState(false);
    auto visible = UseState(true);
    return Column {
      Button("Change").OnClick([changed]() mutable { changed = !changed; }),
      Button("Toggle").OnClick([visible]() mutable { visible = !visible; }),
      Scope([=]() -> View {
        if(!visible) return View{};
        return QRCode(changed ? "new content" : "old content")
            .With(Frame{.width = changed ? 220.0F : 180.0F, .height = 180.0F},
                  Semantics{.label = StringVariant(changed ? "Updated code" : "Original code")}).Key("qr");
      }),
    };
  }, {.window = {.initial_size = {640, 480}}, .show_debug_overlay = false});
  UiTest ui(app);
  CHECK(ui.Find(UiSelector::Key("qr")).One().size == (Size{180, 180}));
  CHECK(ui.FindSemantics(UiSemanticSelector::Label("Original code")).Exists());
  CHECK(!ui.FindSemantics(UiSemanticSelector::Label("old content")).Exists());
  const auto before = ui.CaptureSnapshot();
  ui.Find(UiSelector::Text("Change")).Tap();
  CHECK(ui.Find(UiSelector::Key("qr")).One().size == (Size{220, 180}));
  CHECK(ui.FindSemantics(UiSemanticSelector::Label("Updated code")).Exists());
  CHECK(!ui.FindSemantics(UiSemanticSelector::Label("Original code")).Exists());
  CHECK(!(before == ui.CaptureSnapshot()));
  ui.Find(UiSelector::Text("Toggle")).Tap();
  CHECK(!ui.Find(UiSelector::Key("qr")).Exists());
  ui.Find(UiSelector::Text("Toggle")).Tap();
  CHECK(ui.Find(UiSelector::Key("qr")).Exists());
  ui.PumpAndSettle();
#endif
}); }
