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

int main() { return Run([] {
  const auto symbol = Encode("HUXERUI-2026");
  CheckPaint(symbol, 241, 320);
  CheckPaint(symbol, 400, 201);
  CheckPaint(Encode("HUXERUI-2026", {.format = BarcodeFormat::Code128}), 640, 160);
  (void)QRCode("Hello");
  (void)Barcode("123456");
  (void)Barcode(symbol);
  Throws<std::invalid_argument>([] { (void)QRCode("x", {.format = BarcodeFormat::Code128}); });
  Throws<std::invalid_argument>([&] { (void)Barcode(symbol, {.size = {0, 100}}); });
  Throws<std::invalid_argument>([&] { (void)Barcode(symbol, {.size = {100, -1}}); });
  Throws<std::invalid_argument>([&] { (void)Barcode(symbol, {.size = {100, 16385}}); });
  Throws<std::invalid_argument>([&] {
    (void)Barcode(symbol, {.size = {std::numeric_limits<float>::quiet_NaN(), 100}});
  });
  Throws<std::invalid_argument>([&] { (void)Barcode(symbol, {.foreground = {2, 0, 0, 1}}); });
  Throws<std::invalid_argument>([&] {
    (void)Barcode(symbol, {.background = {0, 0, 0, std::numeric_limits<float>::infinity()}});
  });

#ifdef QRCODE_HAS_UI_TESTING
  using namespace huxerui::testing;
  Application app([]() -> View {
    auto changed = UseState(false);
    auto visible = UseState(true);
    return Column {
      Button("Change").OnClick([changed]() mutable { changed = !changed.Get(); }),
      Button("Toggle").OnClick([visible]() mutable { visible = !visible.Get(); }),
      Scope([=]() -> View {
        if(!visible.Get()) return View{};
        return QRCode(changed.Get() ? "new content" : "old content", {},
                       {.size = {180, 180}, .label = changed.Get() ? "Updated code" : "Original code"}).Key("qr");
      }),
    };
  }, {.window = {.initial_size = {640, 480}}, .show_debug_overlay = false});
  UiTest ui(app);
  CHECK(ui.Find(UiSelector::Key("qr")).One().size == (Size{180, 180}));
  CHECK(ui.FindSemantics(UiSemanticSelector::Label("Original code")).Exists());
  CHECK(!ui.FindSemantics(UiSemanticSelector::Label("old content")).Exists());
  const auto before = ui.CaptureSnapshot();
  ui.Find(UiSelector::Text("Change")).Tap();
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
