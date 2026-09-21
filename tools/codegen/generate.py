#!/usr/bin/env python3
# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Emit committed sources from YAML using per-use-case Jinja templates."""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from infra.context import build_context
from infra.emit import EMIT, PER_CLASS
from infra.render import make_env, render, write_file


def run_protoc(root: pathlib.Path) -> None:
    protoc = shutil.which("protoc")
    if not protoc:
        raise SystemExit("protoc is required to emit include/vti")
    out = root / "include" / "vti"
    out.mkdir(parents=True, exist_ok=True)
    proto = root / "proto"
    subprocess.check_call(
        [
            protoc,
            f"--cpp_out={out}",
            f"-I{proto}",
            str(proto / "vedam" / "types.proto"),
            *[str(p) for p in sorted((proto / "vedam").glob("*.proto")) if p.name != "types.proto"],
        ]
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=pathlib.Path, default=ROOT)
    args = ap.parse_args()
    root = args.root.resolve()
    ctx = build_context(root)
    env = make_env(root / "tools/codegen/templates")
    for tmpl, dest in EMIT:
        write_file(root / dest, render(env, tmpl, ctx))
    for cls in ctx["classes"]:
        one = dict(ctx)
        one.update(cls)
        for tmpl, pattern in PER_CLASS:
            dest = pattern.format(name=cls["class_name"])
            write_file(root / dest, render(env, tmpl, one))
    run_protoc(root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
