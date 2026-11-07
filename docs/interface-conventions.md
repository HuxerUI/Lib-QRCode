# Interface conventions from FinalScave's libraries

These are observations from public headers and preview projects authored by Scave / FinalScave, recorded on 2026-11-05. They are a reference for Lib-QRCode design, rather than a claim that every HuxerUI extension has an identical API. Author and committer metadata were checked; FarnaHerry's extensions and later contributions were not used as the primary baseline.

## Reference implementations

| Library | Reviewed revision | Main reference |
| --- | --- | --- |
| Lib-Lottie | `7dad8008` | [Public header](https://github.com/HuxerUI/Lib-Lottie/blob/7dad8008/include/huxerui/lottie.h), [preview project](https://github.com/HuxerUI/Lib-Lottie/blob/7dad8008/examples/preview/CMakeLists.txt) |
| Lib-Charts | `e281c5e3` | [Public header](https://github.com/HuxerUI/Lib-Charts/blob/e281c5e3/include/huxerui/charts.h) |
| Lib-Camera | `cada0539` | [Public header](https://github.com/HuxerUI/Lib-Camera/blob/cada0539/include/huxerui/camera.h) |
| Lib-Media | `cf1f31b7` | [Player header](https://github.com/HuxerUI/Lib-Media/blob/cf1f31b7/include/huxerui/mediaplayer.h), [media library header](https://github.com/HuxerUI/Lib-Media/blob/cf1f31b7/include/huxerui/medialibrary.h) |
| Lib-Live2D | `936694bc` | [Public header](https://github.com/HuxerUI/Lib-Live2D/blob/936694bc/include/huxerui/live2d.h) |
| Lib-WebView | `d0782037` | [Public header before later contributor changes](https://github.com/HuxerUI/Lib-WebView/blob/d0782037/include/huxerui/webview.h) |

## Observed public API patterns

| Area | Observed convention | Implication for Lib-QRCode |
| --- | --- | --- |
| Public identity | Headers live under `include/huxerui/`; consumers link `HuxerUI::Name`. Several libraries use a feature namespace; Charts and WebView use `huxerui` directly. | Keep `<huxerui/qrcode.h>`, `HuxerUI::QRCode`, and `huxerui::qrcode`. A namespace migration is not required to match the author. |
| Component form | Lottie, CameraPreview, VideoSurface, and ModelView return ordinary `View` values. Charts provides concrete component types with fluent configuration. | Functions returning `View` are consistent with the baseline. Introduce a component class only if its configuration benefits from a fluent API. |
| Layout | Lottie, CameraPreview, and VideoSurface ask callers to supply bounded layout through parents or `Frame`. Presentation configuration controls fitting and appearance. | Views impose no intrinsic frame. Callers supply bounded layout through a parent or `.With(Frame{...})`. Module count and raster-export scale remain separate from layout DIPs. |
| Configuration | Types and methods use PascalCase; fields use snake_case. Options describe one responsibility, define defaults and validation, and frequently provide value equality. | `QRCodeOptions` contains QR encoding controls; `BarcodeStyle` contains paint colors. These and the codec option types provide value equality. Barcode text declarations require explicit `EncodeOptions`. |
| UI text | Charts uses `StringVariant` for titles, category labels, and accessibility summaries. Literal text and generated string resources share one interface. | Accessible labels use the SDK's `Semantics` modifier and its `StringVariant` support. The default label is the format name. Barcode payload text remains encoding data. |
| Data ownership | Charts snapshots and Live2D assets own their contents and share immutable storage. Public documentation describes borrowed references and stable identity. | Retain the immutable shared `Symbol`; document span lifetime and worker/UI ownership explicitly. Keep ZXing types out of public headers. |
| State and commands | Selection and playback intent are application-controlled. Camera and Media retain live sessions through composition hooks. Command handles have explicit connection and unmount rules. | QR rendering needs no controller or retained device session. If continuous camera scanning is added, define its ownership and cancellation separately from a static symbol view. |
| Events | Mounted views such as Lottie and Charts use typed event keys with `.On<Events::Key>()`. Media session subscriptions use lifecycle-bound callbacks instead. | Use typed events for future mounted component notifications. Synchronous codec functions should continue returning results directly. Do not invent events just to resemble another library. |
| Errors | Invalid caller configuration throws `std::invalid_argument`; invariant/lifetime misuse commonly uses `std::logic_error`. Operational async failures use typed results, status, or events. Diagnostic messages are not stable error codes. | An ordinary decode miss can remain an empty result. If operational loading or scanning errors are introduced, define categories and ownership rather than requiring applications to parse strings. |
| Async operations | Camera, Media, and Live2D expose lazy `Task` operations and document UI-thread affinity, cancellation, and independent result storage. | Keep existing CPU-only codec calls explicitly synchronous. Add task APIs only with a defined scheduling and cancellation contract. |
| Installation | Lottie and Media install application-level services; Camera and the reviewed WebView install root-level modules. Charts and Live2D do not require a root installation hook. | The stateless QR codec and Canvas view need no `Install` hook. Installation depends on resources and lifecycle, not on being a Lib. |
| Documentation | Public headers explain valid ranges, defaults, units, source identity, thread requirements, recomposition, and unmount behavior where relevant. | Document the two view paths separately: immediate text encoding and prepared-symbol rendering. Avoid hidden differences between overload defaults. |

## Project and preview conventions

The reviewed Lottie and Charts previews are independent app projects under `examples/preview`. They use `src/app.cpp` for shared composition, platform entry files for desktop hosts, a project integration CMake file, and resources when needed. Their targets use the `example_` prefix, call `huxerui_add_app`, configure the SDK app integration, and consume the public library target through `huxerui_use_library`.

Library discovery returns early for `HUXERUI_PROJECT_PLAN_ONLY` and `HUXERUI_LIBRARY_GRAPH_ONLY`, before building native dependencies. Preview and library metadata must describe their actual targets and namespaces. SDK scaffolds and installation hook signatures vary by revision; compare against the project's tested SDK before adopting a newer template.

Lib-QRCode has an independent `examples/preview` build entry, an `example_qrcode` target, shared composition in `src/app.cpp`, a separate application declaration, platform entry files, and its own `HuxerUIProject.cmake`. Its metadata names the actual target and library namespace. Platform entry files do not replace the SDK's platform shells or packaging configuration; only the Windows host build has been verified locally.

## Public interface

```cpp
QRCode("https://github.com/HuxerUI", {.error_correction = ErrorCorrection::High})
    .With(Frame{.width = 240.0F, .height = 240.0F}, Semantics{.label = "Ticket code"});

Barcode("HUXERUI-2026", {.format = BarcodeFormat::Code128})
    .With(Frame{.width = 480.0F, .height = 120.0F});

auto symbol = Encode("prepared");
QRCode(symbol).With(Frame{.width = 240.0F, .height = 240.0F});
```

All view overloads share the same black-on-white paint defaults and impose no layout size. Prepared symbols compare by shared allocation identity; module spans remain valid while a symbol retains that allocation. The library requires no controller or installation hook. Static codec outcomes are returned directly; no mounted events or asynchronous scheduling contract is invented for synchronous encoding and decoding.
