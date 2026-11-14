#!/usr/bin/env python3
# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Validate model/ and emit the generated C, C++, protobuf, and protoc output."""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from infra.context import build_context  # noqa: E402
from infra.render import make_env, render_tree, write_file  # noqa: E402
from infra.schema import SchemaError  # noqa: E402

PROTOC_OUT = pathlib.Path("include/vti")


def run_protoc(proto_root: pathlib.Path, files: list[str], out: pathlib.Path, grpc: bool) -> None:
    """Compile @p files under @p proto_root into @p out, with gRPC stubs when asked."""
    protoc = shutil.which("protoc")
    if not protoc:
        raise SystemExit("protoc is required to emit include/vti")
    cmd = [protoc, f"--cpp_out={out}", f"-I{proto_root}"]
    if grpc:
        plugin = shutil.which("grpc_cpp_plugin")
        if not plugin:
            raise SystemExit(
                "--grpc needs grpc_cpp_plugin on PATH. Install the gRPC C++ tools, "
                "or run without --grpc."
            )
        cmd += [f"--grpc_out={out}", f"--plugin=protoc-gen-grpc={plugin}"]
    out.mkdir(parents=True, exist_ok=True)
    subprocess.check_call(cmd + files)


def proto_inputs(tree: dict[str, str]) -> list[str]:
    """Paths of the rendered .proto files, relative to proto/."""
    return sorted(p[len("proto/"):] for p in tree if p.startswith("proto/") and p.endswith(".proto"))


def check(root: pathlib.Path, tree: dict[str, str], grpc: bool) -> int:
    """Compare the rendered tree and the protoc output with the files on disk."""
    differ = []
    for rel, text in sorted(tree.items()):
        path = root / rel
        if not path.is_file() or path.read_text(encoding="utf-8") != text:
            differ.append(rel)
    with tempfile.TemporaryDirectory() as tmp:
        tmp_root = pathlib.Path(tmp)
        for rel, text in tree.items():
            if rel.startswith("proto/"):
                write_file(tmp_root / rel, text)
        out = tmp_root / "out"
        run_protoc(tmp_root / "proto", proto_inputs(tree), out, grpc)
        for made in sorted(p for p in out.rglob("*") if p.is_file()):
            rel = PROTOC_OUT / made.relative_to(out)
            path = root / rel
            if not path.is_file() or path.read_bytes() != made.read_bytes():
                differ.append(str(rel))
    for rel in differ:
        print(f"differs: {rel}")
    return 1 if differ else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", type=pathlib.Path, default=ROOT)
    ap.add_argument("--check", action="store_true", help="compare with the tree and write nothing")
    ap.add_argument("--grpc", action="store_true", help="also emit gRPC C++ service stubs")
    args = ap.parse_args()
    root = args.root.resolve()
    try:
        ctx = build_context(root)
    except SchemaError as err:
        print("model errors:", file=sys.stderr)
        print(err, file=sys.stderr)
        return 1
    tree = render_tree(make_env(root / "tools/codegen/templates"), ctx)
    if args.check:
        return check(root, tree, args.grpc)
    for rel, text in tree.items():
        write_file(root / rel, text)
    run_protoc(root / "proto", proto_inputs(tree), root / PROTOC_OUT, args.grpc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
