# Third-party notices

## ZXing-C++

- Upstream: https://github.com/zxing-cpp/zxing-cpp
- Version: 2.3.0, commit `d6068bcebeb8fd9f0d35a99b00d202be86a14dbe`
- Location: `third_party/zxing/core`
- License: Apache-2.0; see [LICENSE](third_party/zxing/LICENSE) and per-file copyright notices.

The core sources and CMake files are vendored with one local fix: `PDFHighLevelEncoder.cpp` uses `int64_t` rather than `long` for its 48-bit byte-compaction accumulator, preventing truncation on Windows (LLP64). The unused `core/src/libzint` symlink directory is excluded. Only the built-in writers (`ZXING_WRITERS=OLD`) and readers are enabled; no libzint, network fetch, or experimental API is used. Upstream tests, examples, wrappers, and top-level build files are not distributed. This intentionally pins the 2.x built-in writer API rather than the newer libzint backend.

To update, fetch an upstream revision into a temporary checkout, copy its `core` directory and root `LICENSE`, exclude `core/src/libzint`, preserve the accumulator fix if still needed, update this revision, and run the codec and view test suites before committing.

## libzueci

Included by ZXing-C++ under `third_party/zxing/core/src/libzueci`. Copyright (C) 2022 gitlost. BSD-3-Clause; see [license text](third_party/zxing/LICENSE.libzueci). Character mapping files retain their individual copyright and license notices.
