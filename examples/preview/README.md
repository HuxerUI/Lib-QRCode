# QRCode preview

This demo follows the other HuxerUI Lib projects: `examples/preview`, an `example_qrcode` target created by `huxerui_add_app`, and `huxerui_use_library` linked to the library's public target. Shared application content lives in `src/app.cpp`; `src/application.cpp` declares the application. Platform entry files under `platform/` call `RunApplication()`. `HuxerUIProject.cmake` describes the independent app plan and initializes its compiler.

Select an enabled writer, edit the payload, set QR correction, load samples, switch themes, or export SVG through the system save dialog. The preview uses a vertical layout on narrow screens. Invalid payloads show an error and retain the previous valid preview. **Decode reference image** reads a bundled PGM made from the independently generated [QR fixture](../../tests/fixtures/qr_reference.h). **Decode preview** reads the current generated symbol.

Build from the repository root:

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI -DHUXERUI_QRCODE_BUILD_EXAMPLES=ON
cmake --build build --config Release --target example_qrcode --parallel
```

Or build the demo as an independent application:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

On Visual Studio generators the root build produces `build/examples/preview/Release/example_qrcode.exe`. The independent build produces `build-preview/Release/example_qrcode.exe`. Export is enabled when the HuxerUI host supports a file-save picker.

## Android and iOS

Run these commands from `examples/preview` with the HuxerUI CLI and the corresponding platform tools installed. Set `/path/to/HuxerUI` to the pinned source checkout or omit `--source` to use an installed SDK.

```sh
huxerui package android --profile debug --source /path/to/HuxerUI
huxerui run ios --device <simulator-id> --profile debug --source /path/to/HuxerUI
```

Android uses Gradle, JDK 17, SDK 36 and NDK 29.0.14206865. The APK is written to `dist/android/`. iOS uses Xcode with an available iPhone Simulator. List destinations with `huxerui devices ios`. For direct Xcode builds, copy `platform/ios/Config/Local.xcconfig.example` to `Local.xcconfig` and set `HUXERUI_HOME`; the CLI supplies both automatically.

The [mobile workflow](../../.github/workflows/mobile.yml) builds an Android `arm64-v8a` APK for devices and an `x86_64` APK for the emulator. It also builds the iOS Simulator app. Both simulator jobs install the preview, launch it and assert that its bundled image decodes to `HuxerUI external fixture`. The Windows UI test exercises the reference-image button at desktop and phone widths.

### Device validation

On an Android phone, install the arm64 APK. On an iPhone, select an iOS device with `huxerui run ios --device <device-id>` and configure signing in Xcode. In portrait and landscape, check that the preview is visible without horizontal scrolling, decode the reference image, and use the other device to scan a QR code rendered by the preview. Check that system SVG export succeeds and that canceling it returns to the preview.

Live-camera verification requires a host that supplies camera frames to `Decode`. On both devices, verify permission denial and grant, background/foreground recovery, and suppression of repeated detections. Record capture-to-result p50 and p95 latency on a named device at a fixed frame size. The simulator smoke checks cover app startup, bundled-resource loading and decode. Camera capture and real-device timing require device testing.
