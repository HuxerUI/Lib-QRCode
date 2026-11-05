# Lib-QRCode

[English](README.md)

[HuxerUI](https://github.com/HuxerUI/HuxerUI) 的原生二维码／条码扩展，提供生成、声明式显示、SVG／灰度图导出和图像识别。使用 C++20，命名空间为 `huxerui::qrcode`，CMake 目标为 `HuxerUI::QRCode`。

支持生成 QR Code、Code 128/39/93、EAN-8/13、UPC-A/E、ITF、Codabar、Data Matrix、Aztec 和 PDF417；还可以识别 Micro QR、rMQR、MaxiCode、GS1 DataBar/Expanded/Limited 和 DX Film Edge。

## 接入项目

```cmake
# 在 huxerui_add_app(your_app ...) 之后调用：
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

`QRCode(text, QRCodeOptions)` 固定生成 QR Code，默认 Medium 纠错等级；`Barcode(text, EncodeOptions)` 显式接收编码配置。两者均支持预生成的 `Symbol`，其中 `QRCode(symbol)` 会校验码制。所有视图重载均不内置尺寸，由父布局或 `.With(Frame{...})` 提供边界。`BarcodeStyle` 只控制前景与背景颜色，默认黑白配色在深浅主题下保持一致。原生视图使用 `Canvas`，保持比例，空间足够时按整数 DIP 模块绘制。

默认留白：QR 四模块，一维条码十模块，其他二维格式两模块，均应用到四周。`quiet_zone=-1` 使用默认值，也可设置 0..256。保持留白完整，避免过小尺寸、裁剪、变形或覆盖图案影响扫描。

## 独立编解码和导出

```cpp
#include <huxerui/qrcode/codec.h>

using namespace huxerui::qrcode;

auto symbol = Encode("二维码 / 😀");
auto image = symbol.Rasterize(8);
auto svg = symbol.ToSvg({.scale = 8});
auto decoded = Decode(image.View());

auto results = DecodeAll({rgba_bytes, width, height, PixelFormat::RGBA8, row_stride}, {
  .formats = {BarcodeFormat::QRCode, BarcodeFormat::Code128},
  .max_symbols = 8,
});
```

`Symbol` 使用共享不可变模块数据，可在工作线程生成，然后在 UI 中使用 `Barcode(symbol).With(Frame{...})` 重用。符号副本按共享数据身份比较相等；不同 `Encode` 调用生成不同身份。文本视图重载每次调用都会重新编码。模块数据包括留白，按行存储，1 为深色、0 为浅色；借用的模块 span 在任一符号副本保留该存储期间有效。

识别支持 Gray8、RGB、BGR、RGBA 和 BGRA，支持行尾填充；`row_stride=0` 表示紧密排列。输入是同步借用的 `std::span<const std::uint8_t>`，调用期间必须保持有效。结果拥有 UTF-8 文本、原始字节、码制、角点、方向、镜像／反色信息等。未识别到返回空值；非法参数抛出 `std::invalid_argument`。Alpha 不参与识别，透明图需先合成背景。

输入文本必须是 1..8192 字节的合法 UTF-8，各码制另有容量和字符限制。条码高度可设 1..1024 模块。EAN/UPC 支持后端的校验位校验／补全规则，识别返回规范形式。光栅图和识别图像上限为 64 Mi 像素。相机捕获、权限、文件加载、YUV 转换、后台任务与扫描去重由应用负责；连续且像素步长为一的亮度平面可作为 Gray8 传入。

无障碍默认只提供码制名称作为图像标签，不自动朗读载荷；通过 `.With(Semantics{.label = ...})` 设置应用需要的描述，支持 `StringVariant` 和本地化字符串资源。`QRCodeOptions`、`BarcodeStyle` 与编解码配置均支持值比较。

## 构建与测试

需要 CMake 3.20+ 和 C++20；原生视图还需要 HuxerUI 源码或 SDK。已验证的框架版本为 `cadb1696cfa7b87d9d2d316b3ecba07b85a46962`。

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_QRCODE_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_QRCODE_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

示例遵循其他 Lib 的结构，位于 [`examples/preview`](examples/preview/README.md)，目标为 `example_qrcode`，共享界面位于 `src/app.cpp`，平台入口位于 `platform/`。支持选择 13 种码制、编辑载荷、设置 QR 纠错等级、载入合法示例／Unicode 示例、切换主题、识别预览，并通过系统保存对话框导出 SVG。输入不合法时显示错误并保留上次有效预览。

也可以单独构建 Demo：

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

独立核心不依赖 HuxerUI：

```sh
cmake -S . -B build-core -DHUXERUI_QRCODE_BUILD_VIEW=OFF \
  -DHUXERUI_QRCODE_BUILD_TESTS=ON
cmake --build build-core --config Release --parallel
ctest --test-dir build-core -C Release --output-on-failure
```

核心内置固定版本的 ZXing-C++ 2.3.0，无需联网下载，并修复了 Windows PDF417 48 位字节压缩累加器被 32 位 `long` 截断的问题。CMake 项目请避免再引入另一个独立配置的 ZXing 副本；当前通过 `add_subdirectory` 或 `huxerui_use_library` 接入，不提供独立安装包。

本地验证平台为 Windows；CI 覆盖 Linux、macOS、Windows 核心，以及 Windows 原生视图。其他 HuxerUI 后端未在本项目实测。完整 API、输入限制和行为约定见 [英文文档](README.md)。

公开接口按 FinalScave 的库风格设计，依据见 [接口约定](docs/interface-conventions.md)。

## 许可

本组件为 MIT；ZXing-C++ 为 Apache-2.0，内含 BSD-3-Clause 的 libzueci。详见 [第三方声明](THIRD_PARTY_NOTICES.md)。
