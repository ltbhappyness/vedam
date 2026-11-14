# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Check model/types.yaml and every class file against model/schema/ and the cross-file rules."""

from __future__ import annotations

import pathlib

import jsonschema
import yaml

# The room a transport keeps per attribute. struct_codec.cc asserts it equals VTI_CODEC_VALUE_MAX.
VALUE_SLOT_BYTES = 256

# Bytes in one item of a builtin kind. struct_codec.cc asserts each list against sizeof.
_KIND_BYTES = {"BOOL": 1, "S32": 4, "U32": 4, "U64": 8, "VERSION": 8}


class SchemaError(Exception):
    """Every problem found in the model, one per line."""


def load_yaml(path: pathlib.Path) -> object:
    with path.open(encoding="utf-8") as fh:
        return yaml.safe_load(fh)


def _json_path(error: jsonschema.ValidationError) -> str:
    parts = ["$"]
    for item in error.absolute_path:
        parts.append(f"[{item}]" if isinstance(item, int) else f".{item}")
    return "".join(parts)


def _schema_errors(schema: dict, doc: object, path: pathlib.Path, root: pathlib.Path) -> list[str]:
    validator = jsonschema.Draft202012Validator(schema)
    out = []
    for error in sorted(validator.iter_errors(doc), key=lambda e: list(e.absolute_path)):
        out.append(f"{path.relative_to(root)}: {_json_path(error)}: {error.message}")
    return out


def proto_type_name(c_type: str) -> str:
    """Pascal-case a C type for a protobuf name: drop vi_, drop _t or _e."""
    name = c_type
    if name.startswith("vi_"):
        name = name[3:]
    if name.endswith("_t") or name.endswith("_e"):
        name = name[:-2]
    return "".join(part.capitalize() for part in name.split("_") if part)


def _class_types(cls: dict) -> dict[str, dict]:
    out = {}
    for enum in cls.get("enums") or []:
        out[enum["c_type"]] = {"form": "enum", **enum}
    for struct in cls.get("structs") or []:
        out[struct["c_type"]] = {"form": "struct", **struct}
    return out


def _check_types_file(types: dict, where: str) -> list[str]:
    errors = []
    kinds = {k["name"] for k in types["kinds"]}
    for name, kind in types["builtin_types"].items():
        if kind not in kinds:
            errors.append(f"{where}: $.builtin_types.{name}: kind {kind} is not in kinds")
    ids = [c["id"] for c in types["classes"]]
    for cid in sorted({i for i in ids if ids.count(i) > 1}):
        errors.append(f"{where}: $.classes: class id {cid} is used twice")
    names = [c["name"] for c in types["classes"]]
    for name in sorted({n for n in names if names.count(n) > 1}):
        errors.append(f"{where}: $.classes: class name {name} is used twice")
    for key in ("kinds", "attr_flags", "verbs"):
        seen = [row["name"] for row in types[key]]
        for name in sorted({n for n in seen if seen.count(n) > 1}):
            errors.append(f"{where}: $.{key}: {name} is listed twice")
    return errors


def _check_enum(where: str, i: int, enum: dict) -> list[str]:
    errors = []
    values = [v["value"] for v in enum["values"]]
    for dup in sorted({v for v in values if values.count(v) > 1}):
        errors.append(f"{where}: $.enums[{i}]: value {dup} is used twice in {enum['c_type']}")
    names = [v["name"] for v in enum["values"]]
    for dup in sorted({n for n in names if names.count(n) > 1}):
        errors.append(f"{where}: $.enums[{i}]: name {dup} is used twice in {enum['c_type']}")
    return errors


def _check_struct(where: str, i: int, struct: dict, builtin: dict, local: dict) -> list[str]:
    errors = []
    for j, field in enumerate(struct["fields"]):
        ftype = field["type"]
        if ftype not in builtin and ftype not in local:
            errors.append(f"{where}: $.structs[{i}].fields[{j}].type: {ftype} does not resolve")
        elif ftype == struct["c_type"]:
            errors.append(f"{where}: $.structs[{i}].fields[{j}].type: a struct cannot hold itself")
    return errors


def _check_proto_names(where: str, cls: dict) -> list[str]:
    errors = []
    owner: dict[str, str] = {}
    title = cls["class"].capitalize()
    owner[f"{title}Attr"] = "the attribute enum"
    owner[f"{title}Service"] = "the service"
    for key in ("enums", "structs"):
        for i, item in enumerate(cls.get(key) or []):
            name = item.get("proto_name") or proto_type_name(item["c_type"])
            here = f"$.{key}[{i}] ({item['c_type']})"
            if name in owner:
                errors.append(f"{where}: protobuf name {name} is used by {owner[name]} and {here}")
            else:
                owner[name] = here
    return errors


def _check_list(here: str, attr: dict, kind: str) -> list[str]:
    """Refuse a text item and a list whose items do not fit one value slot."""
    if kind == "TEXT":
        return [f"{here}.list: a list item cannot be {attr['type']}; use a TEXT attribute"]
    item = _KIND_BYTES.get(kind)
    count = attr["list"]["max_count"]
    if item is not None and count * item > VALUE_SLOT_BYTES:
        return [
            f"{here}.list.max_count: {count} items of {item} bytes need {count * item} bytes, "
            f"more than one value slot of {VALUE_SLOT_BYTES} bytes"
        ]
    return []


def _check_attr(where: str, name: str, attr: dict, kind_of: dict, local: dict, types: dict) -> list[str]:
    errors = []
    here = f"{where}: $.attrs.{name}"
    flags = {f["name"] for f in types["attr_flags"]}
    for bucket in ("flags_public", "flags_internal"):
        for flag in attr.get(bucket) or []:
            if flag not in flags:
                errors.append(f"{here}.{bucket}: {flag} is not in types.yaml attr_flags")
    kind = kind_of.get(attr["type"])
    if kind is None:
        errors.append(f"{here}.type: {attr['type']} does not resolve")
        return errors
    integer = {k["name"]: k["integer"] for k in types["kinds"]}
    rng = attr.get("range")
    if rng is not None:
        if not integer.get(kind, False):
            errors.append(f"{here}.range: kind {kind} is not an integer kind")
        elif rng["min"] > rng["max"]:
            errors.append(f"{here}.range: min {rng['min']} is above max {rng['max']}")
    if "max_len" in attr and kind != "TEXT":
        errors.append(f"{here}.max_len: kind {kind} is not TEXT")
    elif attr.get("max_len", 0) > VALUE_SLOT_BYTES:
        errors.append(
            f"{here}.max_len: {attr['max_len']} bytes does not fit one value slot of "
            f"{VALUE_SLOT_BYTES} bytes"
        )
    if "list" in attr:
        errors.extend(_check_list(here, attr, kind))
    target = local.get(attr["type"])
    if attr["visibility"] == "public" and target and target.get("visibility") == "internal":
        errors.append(f"{here}: a public attribute uses the internal type {attr['type']}")
    return errors


def _check_class(path: pathlib.Path, cls: dict, types: dict, root: pathlib.Path) -> list[str]:
    where = str(path.relative_to(root))
    errors = []
    if path.stem != cls["class"]:
        errors.append(f"{where}: $.class: {cls['class']} does not equal the file stem {path.stem}")
    if cls["class"].upper() not in {c["name"] for c in types["classes"]}:
        errors.append(f"{where}: $.class: {cls['class']} is not listed in types.yaml classes")
    verbs = [v["name"] for v in types["verbs"]]
    for verb in cls["verbs"]:
        if verb not in verbs:
            errors.append(f"{where}: $.verbs.{verb}: not a verb in types.yaml")
    for verb in verbs:
        if verb not in cls["verbs"]:
            errors.append(f"{where}: $.verbs: {verb} is missing")
    local = _class_types(cls)
    builtin = types["builtin_types"]
    kind_of = dict(builtin)
    for c_type, item in local.items():
        if c_type in builtin:
            errors.append(f"{where}: {c_type} shadows a builtin type")
        kind_of[c_type] = "S32" if item["form"] == "enum" else "STRUCT"
    for i, enum in enumerate(cls.get("enums") or []):
        errors.extend(_check_enum(where, i, enum))
    for i, struct in enumerate(cls.get("structs") or []):
        errors.extend(_check_struct(where, i, struct, builtin, local))
    errors.extend(_check_proto_names(where, cls))
    ids = [a["id"] for a in cls["attrs"].values()]
    for dup in sorted({i for i in ids if ids.count(i) > 1}):
        errors.append(f"{where}: $.attrs: attribute id {dup} is used twice")
    for name, attr in cls["attrs"].items():
        errors.extend(_check_attr(where, name, attr, kind_of, local, types))
    return errors


def load_and_validate(root: pathlib.Path) -> tuple[dict, list[tuple[pathlib.Path, dict]]]:
    """Load the model, validate it, and raise SchemaError listing every problem."""
    model = root / "model"
    types_schema = load_yaml(model / "schema" / "types.schema.yaml")
    class_schema = load_yaml(model / "schema" / "class.schema.yaml")
    types_path = model / "types.yaml"
    types = load_yaml(types_path)
    errors = _schema_errors(types_schema, types, types_path, root)
    classes = []
    for path in sorted((model / "classes").glob("*.yaml")):
        doc = load_yaml(path)
        found = _schema_errors(class_schema, doc, path, root)
        errors.extend(found)
        if not found:
            classes.append((path, doc))
    if errors:
        raise SchemaError("\n".join(errors))
    errors.extend(_check_types_file(types, str(types_path.relative_to(root))))
    for path, doc in classes:
        errors.extend(_check_class(path, doc, types, root))
    files = {doc["class"].upper() for _, doc in classes}
    for item in types["classes"]:
        if item["name"] not in files:
            errors.append(f"model/types.yaml: $.classes: {item['name']} has no class file")
    if errors:
        raise SchemaError("\n".join(errors))
    return types, classes
