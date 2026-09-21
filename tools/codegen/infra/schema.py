# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Check types.yaml and class YAML against schema/vedam.yaml."""

from __future__ import annotations

import pathlib

import yaml

INTERNAL_TYPES = {"v_transport_t", "v_transport_ops_t"}


class SchemaError(Exception):
    pass


def load_schema(root: pathlib.Path) -> dict:
    path = root / "tools/codegen/schema/vedam.yaml"
    with path.open(encoding="utf-8") as fh:
        return yaml.safe_load(fh)


def validate(schema: dict, types: dict, classes: list[tuple[pathlib.Path, dict]]) -> None:
    if schema.get("version") != 1:
        raise SchemaError("schema version must be 1")
    for key in ("status", "proto_status", "classes"):
        if key not in types:
            raise SchemaError(f"types.yaml missing {key}")
    verbs = set(schema["verb_names"])
    vis = set(schema["visibility"])
    flags = set(schema["attr_flags"])
    class_names = set()
    class_ids = set()
    for item in types["classes"]:
        if item["id"] in class_ids:
            raise SchemaError(f"duplicate class id {item['id']}")
        class_ids.add(item["id"])
        class_names.add(item["name"].lower())
    seen_files = set()
    for path, cls in classes:
        if path.stem in seen_files:
            raise SchemaError(f"duplicate class file {path.name}")
        seen_files.add(path.stem)
        if path.stem != cls.get("class"):
            raise SchemaError(f"{path.name}: class field must match the file stem")
        if cls["class"] not in class_names:
            raise SchemaError(f"{cls['class']} is not listed in types.yaml classes")
        if cls.get("singleton") not in (True, False):
            raise SchemaError(f"{cls['class']}: singleton must be true or false")
        if cls.get("datastore") not in ("none", "kv", "git"):
            raise SchemaError(f"{cls['class']}: datastore must be none, kv, or git")
        uuid_lines = cls.get("uuid_lines")
        if not isinstance(uuid_lines, list) or not uuid_lines:
            raise SchemaError(f"{cls['class']}: uuid_lines must be a non-empty list")
        for i, line in enumerate(uuid_lines):
            if not isinstance(line, dict):
                raise SchemaError(f"{cls['class']}: uuid_lines[{i}] must be a map")
            for key in ("major", "minor", "commit"):
                if key not in line:
                    raise SchemaError(f"{cls['class']}: uuid_lines[{i}] missing {key}")
            commit = str(line["commit"])
            if len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit):
                raise SchemaError(f"{cls['class']}: uuid_lines[{i}] commit must be 40 lowercase hex digits")
        ids = set()
        for name, verb in (cls.get("verbs") or {}).items():
            if name not in verbs:
                raise SchemaError(f"{cls['class']}: verb {name} is not in the schema")
            if verb.get("visibility") not in vis:
                raise SchemaError(f"{cls['class']}.{name}: bad visibility")
        for name, attr in (cls.get("attrs") or {}).items():
            if attr["id"] in ids:
                raise SchemaError(f"{cls['class']}: duplicate attr id {attr['id']}")
            ids.add(attr["id"])
            if attr.get("visibility") not in vis:
                raise SchemaError(f"{cls['class']}.{name}: bad visibility")
            if "type" not in attr or "comment" not in attr:
                raise SchemaError(f"{cls['class']}.{name}: type and comment are required")
            for bucket in ("flags_public", "flags_internal"):
                for flag in attr.get(bucket) or []:
                    if flag not in flags:
                        raise SchemaError(f"{cls['class']}.{name}: flag {flag}")
            if attr["visibility"] == "public" and attr["type"] in INTERNAL_TYPES:
                raise SchemaError(f"{cls['class']}.{name}: public attribute uses an internal type")
