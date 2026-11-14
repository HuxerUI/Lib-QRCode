# Third-party notices

## ZXing-C++

- Upstream: https://github.com/zxing-cpp/zxing-cpp
- Version: 3.1.1, commit `287c85df6f961c8efbfb5ffd736cd9457b8b890e`
- Location: `third_party/zxing/core`
- License: Apache-2.0; see [LICENSE](third_party/zxing/LICENSE) and per-file copyright notices.

The core sources and CMake files include two local changes. `PDFHighLevelEncoder.cpp` uses `int64_t` for the 48-bit byte-compaction accumulator to prevent truncation on Windows (LLP64). The core CMake file accepts `ZXING_LIB_TYPE=OBJECT` so the iOS library archive can carry its backend into the application core.

This integration enables the built-in writers (`ZXING_WRITERS=OLD`) and readers. It keeps `MultiFormatWriter` so encoding height and character-set handling stay consistent with the public API. The unused `core/src/libzint` symlink directory, upstream wrappers, tests and examples are omitted. The build uses no libzint or experimental APIs.

To update, copy the upstream `core` directory and root `LICENSE` from the new revision, excluding `core/src/libzint`. Recheck the two local changes, update this revision, then run the full and reduced codec builds, view tests and mobile builds.

## libzueci

Included by ZXing-C++ under `third_party/zxing/core/src/libzueci`. Copyright (C) 2022 gitlost. BSD-3-Clause; see [license text](third_party/zxing/LICENSE.libzueci). Character mapping files retain their individual copyright and license notices.
