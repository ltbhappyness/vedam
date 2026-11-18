#!/usr/bin/env python3
# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Write vedam_config.h from the option schema and one defconfig."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

import yaml

CODEGEN = pathlib.Path(__file__).resolve().parents[1] / "codegen"
sys.path.insert(0, str(CODEGEN))

from infra.copyright import GENERATED_C  # noqa: E402

CLASS_NUMBER = {"A": 1, "B": 2, "C": 3, "D": 4, "E": 5}
OPTION_FIELDS = {"name", "type", "values", "min", "max", "default", "description"}
_NOT_SET = re.compile(r"^#\s*([A-Za-z0-9_]+)\s+is not set\s*$")


def fail(errors: list[str]) -> None:
    """Print every problem and stop."""
    for err in errors:
        print(err, file=sys.stderr)
    raise SystemExit(1)


def coerce_bool(value: object) -> bool | None:
    """Map a schema or defconfig bool to True or False."""
    if value is True or value == "y":
        return True
    if value is False or value == "n" or value == "0":
        return False
    return None


def load_schema(path: pathlib.Path) -> list[dict]:
    """Read the option list and reject a schema that cannot generate a header."""
    raw = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(raw, dict) or not isinstance(raw.get("options"), list):
        fail([f"{path}: options must be a list"])
    names: set[str] = set()
    errors: list[str] = []
    options: list[dict] = []
    for i, opt in enumerate(raw["options"]):
        where = f"{path}: options[{i}]"
        if not isinstance(opt, dict):
            errors.append(f"{where}: not a map")
            continue
        extra = set(opt) - OPTION_FIELDS
        if extra:
            errors.append(f"{where}: unknown field {sorted(extra)}")
        name = opt.get("name")
        kind = opt.get("type")
        if not isinstance(name, str) or not name:
            errors.append(f"{where}: name is missing")
            continue
        if name in names:
            errors.append(f"{where}: duplicate name {name}")
        names.add(name)
        if kind not in ("enum", "bool", "int"):
            errors.append(f"{where}: type {kind!r} is not enum, bool, or int")
            continue
        if not isinstance(opt.get("description"), str) or not opt["description"]:
            errors.append(f"{where}: description is missing")
        if "default" not in opt:
            errors.append(f"{where}: default is missing")
        if kind == "enum":
            values = opt.get("values")
            if not isinstance(values, list) or not values:
                errors.append(f"{where}: values must be a list")
            elif name == "VEDAM_NODE_CLASS" and list(values) != ["A", "B", "C", "D", "E"]:
                errors.append(f"{where}: VEDAM_NODE_CLASS values must be A B C D E")
        elif "values" in opt:
            errors.append(f"{where}: values is only for an enum")
        if kind == "int":
            if not isinstance(opt.get("min"), int) or not isinstance(opt.get("max"), int):
                errors.append(f"{where}: min and max must be integers")
            elif opt["min"] > opt["max"]:
                errors.append(f"{where}: min is above max")
        elif "min" in opt or "max" in opt:
            errors.append(f"{where}: min and max are only for an int")
        options.append(opt)
    if errors:
        fail(errors)
    return options


def parse_defconfig(path: pathlib.Path) -> dict[str, str]:
    """Read key=value lines. `# KEY is not set` is 0. Any other # line is ignored."""
    found: dict[str, str] = {}
    errors: list[str] = []
    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        stripped = line.strip()
        if not stripped:
            continue
        not_set = _NOT_SET.match(stripped)
        if not_set is not None:
            key = not_set.group(1)
            if key in found:
                errors.append(f"{path}:{n}: duplicate key {key}")
                continue
            found[key] = "0"
            continue
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        if "=" not in text:
            errors.append(f"{path}:{n}: expected key=value")
            continue
        key, value = text.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not key:
            errors.append(f"{path}:{n}: empty key")
            continue
        if key in found:
            errors.append(f"{path}:{n}: duplicate key {key}")
            continue
        found[key] = value
    if errors:
        fail(errors)
    return found


def resolve(options: list[dict], chosen: dict[str, str], where: str) -> dict[str, object]:
    """Fill defaults, reject an unknown key or a value outside its range."""
    errors: list[str] = []
    known = {opt["name"] for opt in options}
    for key in sorted(chosen):
        if key not in known:
            errors.append(f"{where}: {key} is not in the schema")
    values: dict[str, object] = {}
    for opt in options:
        name = opt["name"]
        raw = chosen.get(name)
        default = opt["default"]
        kind = opt["type"]
        if kind == "bool":
            if coerce_bool(default) is None:
                errors.append(f"{name}: default must be y or n")
                continue
            if raw is None:
                values[name] = coerce_bool(default)
                continue
            flag = coerce_bool(raw)
            if flag is None:
                errors.append(f"{where}: {name}={raw} is not y or n")
                continue
            values[name] = flag
        elif kind == "enum":
            if not isinstance(default, str) or default not in opt["values"]:
                errors.append(f"{name}: default is outside values")
                continue
            picked = default if raw is None else raw
            if picked not in opt["values"]:
                errors.append(f"{where}: {name}={picked} is outside values")
                continue
            values[name] = picked
        else:
            if isinstance(default, bool) or not isinstance(default, int):
                errors.append(f"{name}: default must be an integer")
                continue
            if raw is None:
                number = default
            else:
                try:
                    number = int(raw, 10)
                except ValueError:
                    errors.append(f"{where}: {name}={raw} is not an integer")
                    continue
            if number < opt["min"] or number > opt["max"]:
                errors.append(f"{where}: {name}={number} is outside {opt['min']}..{opt['max']}")
                continue
            values[name] = number
    if values.get("VEDAM_ENABLE_CONTRACT_ENGINE") is True:
        errors.append("VEDAM_ENABLE_CONTRACT_ENGINE must stay n until an engine is vendored")
    if errors:
        fail(errors)
    return values


def render_header(options: list[dict], values: dict[str, object]) -> str:
    """Render the forced-include header. Bools are 1 or 0. The class is a number."""
    lines = [
        GENERATED_C.rstrip("\n"),
        "",
        "#ifndef VEDAM_CONFIG_H",
        "#define VEDAM_CONFIG_H",
        "",
    ]
    for opt in options:
        name = opt["name"]
        value = values[name]
        if opt["type"] == "bool":
            lines.append(f"#define {name} {1 if value else 0}")
        elif opt["type"] == "int":
            lines.append(f"#define {name} {value}")
        elif name == "VEDAM_NODE_CLASS":
            letter = str(value)
            lines.append(f"#define VEDAM_NODE_CLASS {CLASS_NUMBER[letter]}")
            lines.append(f"#define VEDAM_NODE_CLASS_{letter} 1")
    lines.append("")
    lines.append("#endif /* VEDAM_CONFIG_H */")
    lines.append("")
    return "\n".join(lines)


def render_cmake(values: dict[str, object]) -> str:
    """Render the configure-time facts the source lists branch on."""
    lines = ["# Generated by tools/config/gen_config.py. Do not edit."]
    lines.append(f'set(VEDAM_NODE_CLASS "{values["VEDAM_NODE_CLASS"]}")')
    for name, value in values.items():
        if name == "VEDAM_NODE_CLASS":
            continue
        if isinstance(value, bool):
            lines.append(f"set({name} {1 if value else 0})")
        else:
            lines.append(f"set({name} {value})")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    """Parse arguments, resolve one defconfig, and write the header."""
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--schema", type=pathlib.Path, required=True)
    ap.add_argument("--defconfig", type=pathlib.Path, required=True)
    ap.add_argument("--out", type=pathlib.Path, required=True)
    ap.add_argument("--cmake", type=pathlib.Path)
    args = ap.parse_args()
    options = load_schema(args.schema)
    chosen = parse_defconfig(args.defconfig)
    values = resolve(options, chosen, str(args.defconfig))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(render_header(options, values), encoding="utf-8")
    if args.cmake is not None:
        args.cmake.parent.mkdir(parents=True, exist_ok=True)
        args.cmake.write_text(render_cmake(values), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
