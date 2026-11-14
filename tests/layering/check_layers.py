#!/usr/bin/env python3
# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Check that every folder includes only the headers its layer allows."""

from __future__ import annotations

import pathlib
import re
import sys

SOURCES = (".c", ".cc", ".h")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
PREFIX = re.compile(r"\b(va|via|vta|VA|VIA|VTA)_")
OPS_CALL = re.compile(r"ops->call")
PROTOBUF = re.compile(r"^vedam/.*\.pb\.h$")

# Folder -> include prefixes it may use. A path ending in / allows the whole folder.
ALLOWED = {
    "src/platform": ["platform/"],
    "src/ds": ["ds/", "platform/", "vi/types.h"],
    "src/util": ["util/", "platform/", "vi/types.h"],
    "src/lock": [
        "lock/",
        "ds/",
        "util/",
        "platform/",
        "vii/types.h",
        "vii/meta.h",
        "core/node.h",
    ],
    "src/lib": [
        "lib/",
        "util/",
        "platform/",
        "vi/types.h",
        "vi/class/",
        "vii/types.h",
        "core/node.h",
    ],
    "src/via": ["vii/", "lib/", "lock/store.h", "util/", "platform/", "core/node.h"],
    "src/va": ["vi/class/", "vii/"],
    "src/vta": [
        "vti/",
        "vii/",
        "lock/store.h",
        "ds/",
        "util/",
        "platform/",
        "core/node.h",
    ],
    "include/lib": ["vi/types.h", "vii/types.h"],
    "include/vii": ["vi/", "vii/"],
}

NO_VTI = ("src/va", "src/lock", "src/lib", "include/lib")
CALL_OWNERS = ("include/core/node.h", "src/vta/vta.c")


def is_generated_pb(path: pathlib.Path) -> bool:
    return ".pb." in path.name


def folder_rule(rel: str) -> str | None:
    for folder in ALLOWED:
        if rel == folder or rel.startswith(folder + "/"):
            return folder
    return None


# Headers included by name from a third_party include directory. They are not beside the .c file.
WWLOCK_HEADERS = ("wwlock.h", "wwlock_futex.h")


def include_allowed(folder: str, header: str, path: pathlib.Path) -> bool:
    """Return True when @p header may appear in a file of @p folder."""
    if folder == "src/lock" and header in WWLOCK_HEADERS:
        return True
    if "/" not in header:
        return (path.parent / header).is_file()
    if folder == "src/vta" and PROTOBUF.match(header):
        return True
    for allowed in ALLOWED[folder]:
        if header == allowed or (allowed.endswith("/") and header.startswith(allowed)):
            return True
    return False


def check_file(root: pathlib.Path, path: pathlib.Path) -> list[str]:
    rel = path.relative_to(root).as_posix()
    folder = folder_rule(rel)
    out = []
    for no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = INCLUDE.match(line)
        if match:
            header = match.group(1)
            if folder is not None and not include_allowed(folder, header, path):
                out.append(f"{rel}:{no}: {folder} may not include \"{header}\"")
            if header.startswith("vti/") and rel.startswith(NO_VTI):
                out.append(f"{rel}:{no}: includes a vti/ header")
        if rel.startswith("include/") and PREFIX.search(line):
            out.append(f"{rel}:{no}: an include/ name uses a va_, via_, or vta_ prefix")
        if OPS_CALL.search(line) and rel not in CALL_OWNERS:
            out.append(f"{rel}:{no}: only vedam_node_call and vti_call reach ops->call")
    return out


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    problems = []
    for top in ("include", "src"):
        for path in sorted((root / top).rglob("*")):
            if path.suffix in SOURCES and path.is_file() and not is_generated_pb(path):
                problems.extend(check_file(root, path))
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
