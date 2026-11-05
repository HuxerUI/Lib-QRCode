# QRCode preview

This demo follows the other HuxerUI Lib projects: `examples/preview`, a `huxerui_add_app` executable, and `huxerui_use_library` linked to the library's public target. Shared application content lives in `src/preview.cpp`; `main.cpp` declares the application and calls `RunApplication()`.

Features: select any of the 13 writable formats, edit a controlled text field, choose QR correction, load format-valid samples or a Unicode QR sample, generate and decode the preview, export SVG through the system save dialog, and switch light/dark themes. Invalid payloads show an error and retain the previous valid preview.

Build from the repository root:

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI -DHUXERUI_QRCODE_BUILD_EXAMPLES=ON
cmake --build build --config Release --target qrcode_preview --parallel
```

Or build the demo as an independent application:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

On Visual Studio generators the root build produces `build/examples/preview/Release/qrcode_preview.exe`. The independent build produces `build-preview/Release/qrcode_preview.exe`. Launch from your desktop or terminal. Export is enabled when the HuxerUI host supports a file-save picker.
