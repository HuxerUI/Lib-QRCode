#include "../examples/preview/src/preview.h"
#include <huxerui/testing/ui_test.h>
#include "test.h"

using namespace huxerui;
using namespace huxerui::testing;

class PreviewResources final : public PlatformResources {
public:
  ResourceConfiguration Configuration() const override { return {}; }
  std::optional<InputStream> OpenRead(std::string_view path) override {
    auto stream = File(std::string(QRCODE_TEST_RESOURCES) + "/" + std::string(path)).OpenRead();
    if(!stream.Succeeded()) return std::nullopt;
    return std::move(stream).Value();
  }
};

int main() { return Run([] {
  Application app(qrcode_demo::PreviewApp, {
    .window = {.initial_size = {1100, 840}},
    .show_debug_overlay = false,
  });
  UiTest ui(app, {.viewport = {1100, 840}, .resource_provider = std::make_shared<PreviewResources>()});
  CHECK(ui.Find(UiSelector::Key("symbol-preview")).Exists());
  ui.Find(UiSelector::Text("Decode preview")).Tap();
  CHECK(ui.Find(UiSelector::Text("Decoded QR Code: https://github.com/HuxerUI")).Exists());
  ui.Find(UiSelector::Text("Use Unicode sample")).Tap();
  ui.Find(UiSelector::Text("Decode preview")).Tap();
  CHECK(ui.Find(UiSelector::Text("Decoded QR Code: HuxerUI / 二维码 / 日本語 / 😀")).Exists());
  auto editor = ui.Find(UiSelector::Key("payload"));
  editor.Tap();
  editor.ReplaceText("");
  ui.Find(UiSelector::Text("Generate")).Tap();
  CHECK(ui.Find(UiSelector::Key("status")).One().text->find("Previous preview retained") != std::string::npos);
  CHECK(ui.Find(UiSelector::Key("symbol-preview")).Exists());
  ui.Find(UiSelector::Text("Load sample")).Tap();
  CHECK(ui.Find(UiSelector::Text("Generated QR Code.")).Exists());
  ui.Find(UiSelector::Text("Dark theme")).Tap();
  CHECK(ui.Find(UiSelector::Text("Light theme")).Exists());
  ui.Find(UiSelector::Text("Decode preview")).Tap();
  CHECK(ui.Find(UiSelector::Text("Decoded QR Code: https://github.com/HuxerUI")).Exists());
  ui.PumpAndSettle();
}); }
