#include "app.h"
#include <huxerui/qrcode.h>
#include <qrcode_preview_resources.h>

#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace qrcode_demo {
using namespace huxerui;
using namespace huxerui::qrcode;

namespace {

constexpr std::array candidate_formats{
  BarcodeFormat::QRCode, BarcodeFormat::Code128, BarcodeFormat::Code39, BarcodeFormat::Code93,
  BarcodeFormat::EAN8, BarcodeFormat::EAN13, BarcodeFormat::UPCA, BarcodeFormat::UPCE,
  BarcodeFormat::ITF, BarcodeFormat::Codabar, BarcodeFormat::DataMatrix, BarcodeFormat::Aztec, BarcodeFormat::PDF417,
};

const std::vector<BarcodeFormat> writable_formats = [] {
  std::vector<BarcodeFormat> formats;
  for(auto format : candidate_formats) {
    if(CanEncode(format)) formats.push_back(format);
  }
  return formats;
}();

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

std::string DescribeDecode(const std::optional<DecodeResult>& result) {
  return result ? "Decoded " + std::string(FormatName(result->format)) + ": " + result->text : "No code detected.";
}

std::string DecodeReferenceImage(const RawAsset& reference) {
  // The bundled fixture is a fixed-size grayscale PGM created independently of the demo encoder.
  constexpr std::string_view header = "P5\n132 132\n255\n";
  constexpr int side = 132;
  const auto bytes = reference.ReadString(true);
  if(!bytes.starts_with(header) || bytes.size() != header.size() + side * side) {
    throw std::runtime_error("Invalid reference image.");
  }
  const auto* pixels = reinterpret_cast<const std::uint8_t*>(bytes.data() + header.size());
  return DescribeDecode(Decode({{pixels, side * side}, side, side}));
}

#ifdef QRCODE_PREVIEW_MOBILE_SMOKE
void CheckReferenceOnLaunch(const ApplicationHandle& application, const RawAsset& reference) {
  const File result_file(application.Directories().cache_directory, "qrcode-reference-result.txt");
  if(!result_file.Exists() && !result_file.WriteString(DecodeReferenceImage(reference))) {
    throw std::runtime_error("Could not write the reference image smoke result.");
  }
}
#else
void CheckReferenceOnLaunch(const ApplicationHandle&, const RawAsset&) {}
#endif

} // namespace

[[huxerui::composable]]
View PreviewApp() {
  if(writable_formats.empty()) return Text("No barcode writer is enabled in this build.");
  const auto first_format = writable_formats.front();
  auto input = UseState(TextEditingValue::FromText(Sample(first_format)));
  auto format = UseState(std::size_t{0});
  auto correction = UseState(std::size_t{1});
  auto generated = UseState(Encode(Sample(first_format), {.format = first_format}));
  auto status = UseState(std::string("Ready. Edit the payload and choose Generate."));
  auto decoded_text = UseState(std::string("Choose Decode preview to inspect this symbol."));
  auto failed = UseState(false);
  auto dark = UseState(false);
  auto exporting = UseState(false);
  auto picker = UseService<FilePicker>();
  auto tasks = UseTaskScope();
  const auto application = UseApplication();
  const auto reference = UseRawResource(qrcode_preview::raw::reference_qr_pgm);
  CheckReferenceOnLaunch(application, reference);
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
    const bool compact = UseViewportClass() != ViewportClass::Expanded;
    const Symbol symbol = generated;
    const bool square = symbol.Format() == BarcodeFormat::QRCode || symbol.Format() == BarcodeFormat::DataMatrix ||
                         symbol.Format() == BarcodeFormat::Aztec;
    auto editor = Column {
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
    }.With(compact ? Frame{} : Frame{.width = 310.0F}, Padding(20.0F), Spacing(14.0F),
            CrossAlign(CrossAxisAlignment::Stretch), Background{theme.colors.surface_container_low}, CornerRadius(16.0F));
    auto preview = Column {
      Text(std::string(FormatName(symbol.Format())) + " preview", TextRole::Title),
      Barcode(symbol)
          .With(compact ? Frame{.height = square ? 240.0F : 140.0F}
                        : Frame{.width = square ? 360.0F : 560.0F, .height = square ? 360.0F : 200.0F},
                Semantics{.label = "Generated " + std::string(FormatName(symbol.Format()))})
          .Key("symbol-preview"),
      Text(std::to_string(symbol.Width()) + " × " + std::to_string(symbol.Height()) + " modules, including quiet zone",
            TextRole::Label),
      Flow {
        Button("Decode preview").OnClick([=] {
          const auto image = generated->Rasterize(4);
          decoded_text = DescribeDecode(Decode(image.View(), {.formats = {generated->Format()}}));
        }),
            Button("Decode reference image").With(Enabled(CanDecode(BarcodeFormat::QRCode))).OnClick([=] {
          try {
            decoded_text = DecodeReferenceImage(reference);
          } catch(const std::exception& error) {
            decoded_text = error.what();
          }
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
    }.With(Padding(20.0F), Spacing(18.0F), CrossAlign(CrossAxisAlignment::Center),
            Background{theme.colors.surface}, CornerRadius(16.0F));
    View panels = compact
        ? View(Column {std::move(editor), std::move(preview)}.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Stretch)))
        : View(Row {std::move(editor), std::move(preview).With(Grow{})}.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Start)));
    return Column {
      Row {
        Column {
          Text("QRCode & Barcode", TextRole::Title),
          Text("Generate, inspect, decode and export symbols.", TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        Button(dark ? "Light theme" : "Dark theme").OnClick([=] { dark = !dark; }),
      }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
      ScrollView(std::move(panels)).With(Grow{}, ScrollBar{}).Key("preview-scroll"),
    }.With(Padding(compact ? 12.0F : 24.0F), Spacing(18.0F), CrossAlign(CrossAxisAlignment::Stretch),
           Background{theme.colors.background});
  }));
}

} // namespace qrcode_demo
