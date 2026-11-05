# QRCode preview

This demo follows the other HuxerUI Lib projects: `examples/preview`, an `example_qrcode` target created by `huxerui_add_app`, and `huxerui_use_library` linked to the library's public target. Shared application content lives in `src/app.cpp`; `src/application.cpp` declares the application. Platform entry files under `platform/` call `RunApplication()`. `HuxerUIProject.cmake` describes the independent app plan and initializes its compiler.

Features: select any of the 13 writable formats, edit a controlled text field, choose QR correction, load format-valid samples or a Unicode QR sample, generate and decode the preview, export SVG through the system save dialog, and switch light/dark themes. Invalid payloads show an error and retain the previous valid preview.

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

On Visual Studio generators the root build produces `build/examples/preview/Release/example_qrcode.exe`. The independent build produces `build-preview/Release/example_qrcode.exe`. Launch from your desktop or terminal. Export is enabled when the HuxerUI host supports a file-save picker. The host build and interaction tests are verified on Windows; other platform entry files require their SDK toolchains and remain unverified here.
