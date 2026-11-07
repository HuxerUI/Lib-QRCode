#include "app.h"
#include <huxerui/qrcode.h>

#include <array>
#include <exception>
#include <string>
#include <vector>

namespace qrcode_demo {
using namespace huxerui;
using namespace huxerui::qrcode;

namespace {

constexpr std::array writable_formats{
  BarcodeFormat::QRCode, BarcodeFormat::Code128, BarcodeFormat::Code39, BarcodeFormat::Code93,
  BarcodeFormat::EAN8, BarcodeFormat::EAN13, BarcodeFormat::UPCA, BarcodeFormat::UPCE,
  BarcodeFormat::ITF, BarcodeFormat::Codabar, BarcodeFormat::DataMatrix, BarcodeFormat::Aztec, BarcodeFormat::PDF417,
};

const std::vector<std::string> format_labels = [] {
  std::vector<std::string> labels;
  for(auto format : writable_formats) labels.emplace_back(FormatName(format));
  return labels;
}();

std::string Sample(BarcodeFormat format) {
  switch(format) {
    case BarcodeFormat::QRCode: return "https://github.com/HuxerUI";
    case BarcodeFormat::EAN8: return "96385074";
    case BarcodeFormat::EAN13: return "5901234123457";
    case BarcodeFormat::UPCA: return "012345678905";
    case BarcodeFormat::UPCE: return "01234565";
    case BarcodeFormat::ITF: return "12345678";
    case BarcodeFormat::Codabar: return "A123456B";
    case BarcodeFormat::DataMatrix:
    case BarcodeFormat::Aztec: return "HuxerUI / 二维码 / 😀";
    default: return "HUXERUI-2026";
  }
}

} // namespace

[[huxerui::composable]]
View PreviewApp() {
  auto input = UseState(TextEditingValue::FromText(Sample(BarcodeFormat::QRCode)));
  auto format = UseState(std::size_t{0});
  auto correction = UseState(std::size_t{1});
  auto generated = UseState(Encode(Sample(BarcodeFormat::QRCode)));
  auto status = UseState(std::string("Ready. Edit the payload and choose Generate."));
  auto decoded_text = UseState(std::string("Choose Decode preview to inspect this symbol."));
  auto failed = UseState(false);
  auto dark = UseState(false);
  auto exporting = UseState(false);
  auto picker = UseService<FilePicker>();
  auto tasks = UseTaskScope();
  const auto application = UseApplication();
  const auto generate = [=] {
    try {
      generated = Encode(input->text, {
        .format = writable_formats[format],
        .qr_error_correction = static_cast<ErrorCorrection>(correction.Get()),
      });
      failed = false;
      status = "Generated " + std::string(FormatName(generated->Format())) + ".";
      decoded_text = "Choose Decode preview to inspect this symbol.";
    } catch(const std::exception& error) {
      failed = true;
      status = std::string(error.what()) + " Previous preview retained.";
    }
  };
  return MaterialTheme(dark ? MaterialDarkThemeSpec() : MaterialLightThemeSpec(), Scope([=] {
    const auto& theme = UseTheme();
    const Symbol symbol = generated;
    const bool square = symbol.Format() == BarcodeFormat::QRCode || symbol.Format() == BarcodeFormat::DataMatrix ||
                         symbol.Format() == BarcodeFormat::Aztec;
    return Column {
      Row {
        Column {
          Text("QRCode & Barcode", TextRole::Title),
          Text("Generate, inspect, decode and export symbols.", TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        Button(dark ? "Light theme" : "Dark theme").OnClick([=] { dark = !dark; }),
      }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
      ScrollView(Row {
        Column {
          Text("Create a symbol", TextRole::Title),
          Select(format_labels, format, [](const std::string& label) { return Text(label).Key(label); })
              .Label("Format")
              .OnChanged([=](std::size_t index) {
                format = index;
                input = TextEditingValue::FromText(Sample(writable_formats[index]));
                generate();
              }).Key("format"),
          TextField(input).Label("Payload").Placeholder("Text, URL or barcode digits")
              .LineLimits(TextFieldLineLimits::MultiLine(2, 4))
              .OnChanged([=](const TextEditingValue& value) { input = value; })
              .Key("payload"),
          Text("QR error correction", TextRole::Label),
          SegmentedButton({"L", "M", "Q", "H"}, correction)
              .OnChanged([=](std::size_t index) { correction = index; })
              .With(Enabled(writable_formats[format] == BarcodeFormat::QRCode)),
          Text("Low / Medium / Quartile / High", TextRole::Label),
          Button("Generate").OnClick(generate),
          Button("Load sample").OnClick([=] {
            input = TextEditingValue::FromText(Sample(writable_formats[format]));
            generate();
          }),
          Button("Use Unicode sample").With(Enabled(writable_formats[format] == BarcodeFormat::QRCode))
              .OnClick([=] {
                input = TextEditingValue::FromText("HuxerUI / 二维码 / 日本語 / 😀");
                generate();
              }),
          Text("Keep the white quiet zone clear. Dark mode preserves black-on-white symbol contrast.", TextRole::Label),
        }.With(Frame{.width = 310.0F}, Padding(20.0F), Spacing(14.0F),
                CrossAlign(CrossAxisAlignment::Stretch), Background{theme.colors.surface_container_low}, CornerRadius(16.0F)),
        Column {
          Text(std::string(FormatName(symbol.Format())) + " preview", TextRole::Title),
          Barcode(symbol)
              .With(Frame{.width = square ? 360.0F : 560.0F, .height = square ? 360.0F : 200.0F},
                    Semantics{.label = "Generated " + std::string(FormatName(symbol.Format()))})
              .Key("symbol-preview"),
          Text(std::to_string(symbol.Width()) + " × " + std::to_string(symbol.Height()) + " modules, including quiet zone",
                TextRole::Label),
          Flow {
            Button("Decode preview").OnClick([=] {
              const auto image = generated->Rasterize(4);
              const auto result = Decode(image.View(), {.formats = {generated->Format()}});
              decoded_text = result ? "Decoded " + std::string(FormatName(result->format)) + ": " + result->text : "No code detected.";
            }),
            Button("Export SVG").With(Enabled(!exporting && picker->CanSaveFiles())).OnClick([=] {
              exporting = true;
              const auto svg = generated->ToSvg({.scale = 8});
              tasks.Launch([=]() -> Task<void> {
                try {
                  const File export_file(application.Directories().temporary_directory, "huxerui-code.svg");
                  if(!co_await export_file.WriteStringAsync(svg)) {
                    status = "Could not prepare the SVG export.";
                    failed = true;
                  } else {
                    const bool saved = co_await picker->SaveFileAsync(export_file, {
                      .suggested_name = "huxerui-code.svg",
                      .filter = {.name = "SVG image", .extensions = {"svg"}, .content_types = {"image/svg+xml"}},
                    });
                    status = saved ? "SVG exported." : "SVG export canceled or unavailable.";
                    failed = false;
                  }
                } catch(const std::exception& error) {
                  status = error.what();
                  failed = true;
                }
                exporting = false;
              });
            }),
          }.With(Spacing(12.0F)),
          SelectionArea { Text(decoded_text).Key("decode-result") },
          Text(status).With(Foreground(failed ? theme.colors.error : theme.colors.primary)).Key("status"),
        }.With(Grow{}, Padding(20.0F), Spacing(18.0F), CrossAlign(CrossAxisAlignment::Center),
                Background{theme.colors.surface}, CornerRadius(16.0F)),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Start))).With(Grow{}, ScrollBar{}),
    }.With(Padding(24.0F), Spacing(24.0F), CrossAlign(CrossAxisAlignment::Stretch), Background{theme.colors.background});
  }));
}

} // namespace qrcode_demo
