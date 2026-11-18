#!/usr/bin/env python3
# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Reconfigure one build directory for each class sample, then build and run ctest."""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess
import sys


def samples(root: pathlib.Path) -> list[pathlib.Path]:
    """Class A through D, plus E when emcc is on PATH."""
    letters = ["a", "b", "c", "d"]
    if shutil.which("emcc") is not None:
        letters.append("e")
    return [root / "config" / f"defconfig.class-{letter}" for letter in letters]


def main() -> int:
    """Configure, build, and test each sample in the build directory already passed in."""
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", type=pathlib.Path, required=True)
    ap.add_argument("--build", type=pathlib.Path, required=True)
    args = ap.parse_args()
    root = args.root.resolve()
    build = args.build.resolve()
    dotconfig = root / ".config"
    for cfg in samples(root):
        print(f"class sample {cfg.name}", flush=True)
        before = dotconfig.read_bytes()
        subprocess.run(
            [
                "cmake",
                "-S",
                str(root),
                "-B",
                str(build),
                f"-DVEDAM_CONFIG_FILE={cfg}",
            ],
            check=True,
        )
        if dotconfig.read_bytes() != before:
            print(".config changed", file=sys.stderr)
            return 1
        subprocess.run(["cmake", "--build", str(build)], check=True)
        subprocess.run(
            [
                "ctest",
                "--test-dir",
                str(build),
                "--output-on-failure",
                "-E",
                "^ClassSample$",
            ],
            check=True,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
