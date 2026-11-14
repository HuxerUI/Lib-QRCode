# Independent decode fixtures

The PGM images are generated with Zint 2.15.0, commit `4896136bdbebb1a0908a5cc419513efa5bf4bf70`, independently of the vendored ZXing decoder and writer. Payloads are synthetic test data. `generate.py` records the exact payloads, formats, rendering settings and variant options. Zint and Pillow are only needed for regeneration; building and running the C++ tests needs neither dependency nor network access.

```sh
python3 tests/fixtures/generate.py /path/to/zint-2.15.0
```

The fixture suite checks automatic and explicit format selection in normal and pure modes, decoded bytes and text, positions, symbology identifiers, exclusion filters, and disabled-family filters. It covers Micro QR, rMQR, MaxiCode, GS1 DataBar Omni/Limited/Stacked/Stacked Omni/Expanded/Expanded Stacked, Compact PDF417, Extended Code 39, DX Film Edge, UPC-A and UPC-E. UPC-A is also read with EAN-13 and mixed filters to verify canonicalization. QR Model 1 remains without a fixture.

The separate `qr_reference.h` fixture is generated with python-qrcode 8.2; its source settings are documented in that header. The preview's bundled reference image uses this same fixture.
