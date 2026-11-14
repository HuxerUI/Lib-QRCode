#!/usr/bin/env python3
"""Regenerate independent decode fixtures with Zint 2.15.0 and Pillow."""

import argparse
from pathlib import Path
import subprocess
import tempfile

from PIL import Image


FIXTURES = [
    ("micro_qr", "MICROQR", "1234567", ["--vers=3"]),
    ("rmqr", "RMQR", "HUXERUI-RMQR", []),
    ("maxicode", "MAXICODE", "HUXERUI-MAXICODE", ["--mode=4", "--scale=0.5"]),
    ("databar", "DBAR_OMN", "09501101530003", []),
    ("databar_limited", "DBAR_LTD", "09501101530003", []),
    ("databar_stacked", "DBAR_STK", "09501101530003", []),
    ("databar_stacked_omni", "DBAR_OMNSTK", "09501101530003", []),
    ("databar_expanded", "DBAR_EXP", "[01]09501101530003[10]ABC123", []),
    ("databar_expanded_stacked", "DBAR_EXPSTK", "[01]09501101530003[10]ABC123", ["--cols=2"]),
    ("compact_pdf417", "PDF417COMP", "HUXERUI-COMPACT-PDF417", []),
    ("code39_extended", "EXCODE39", "HuxerUI-2026", []),
    ("dx_film_edge", "DXFILMEDGE", "112-1/10A", []),
    ("upca", "UPCA", "012345678905", []),
    ("upce", "UPCE", "01234565", []),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zint", help="Path to the Zint 2.15.0 executable")
    args = parser.parse_args()
    version = subprocess.check_output([args.zint, "--version"], text=True)
    if not version.startswith("Zint version 2.15.0"):
        parser.error("Fixtures require Zint 2.15.0")
    destination = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory() as directory:
        for name, barcode, payload, options in FIXTURES:
            bitmap = Path(directory) / f"{name}.bmp"
            subprocess.run([
                args.zint, f"--barcode={barcode}", f"--data={payload}",
                "--notext", "--quietzones", "--scale=1", "--whitesp=10", "--vwhitesp=10",
                f"--output={bitmap}", *options,
            ], check=True)
            with Image.open(bitmap) as image:
                image.convert("L").save(destination / f"{name}.pgm")


if __name__ == "__main__":
    main()
