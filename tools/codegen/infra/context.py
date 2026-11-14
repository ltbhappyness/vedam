# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Turn the model into the context every template renders from."""

from __future__ import annotations

import pathlib

from .copyright import COPYRIGHT_C, GENERATED_C, GENERATED_PROTO
from .model import VEDAM_UUID_NS, Model, load_model, wrap
from .schema import VALUE_SLOT_BYTES


# The column limit in .clang-format. A longer enumerator line breaks after "=".
_C_COLUMNS = 100


def status_rows(model: Model) -> list[dict]:
    rows = []
    for s in model.status:
        ident = f"V_STATUS_{s['name']}"
        line = f"    {ident} = {s['value']}, /**< {s['comment']} */"
        rows.append(
            {
                "ident": ident,
                "value": str(s["value"]),
                "comment": s["comment"],
                "split": len(line) > _C_COLUMNS,
            }
        )
    return rows


def proto_status_lines(model: Model) -> list[str]:
    lines = []
    for i, name in enumerate(model.proto_status):
        if name == "UNSPECIFIED":
            lines.append(f"  STATUS_CODE_UNSPECIFIED = {i};")
        elif name == "OK":
            lines.append(f"  STATUS_OK = {i};")
        else:
            lines.append(f"  STATUS_{name} = {i};")
    return lines


def build_context(root: pathlib.Path) -> dict:
    model = load_model(root)
    return {
        "copyright_c": COPYRIGHT_C.rstrip("\n"),
        "generated_c": GENERATED_C.rstrip("\n"),
        "generated_proto": GENERATED_PROTO.rstrip("\n"),
        "model": model,
        "package": model.package,
        "kinds": model.kinds,
        "flags": model.flags,
        "verbs": model.verbs,
        "classes": model.classes,
        "structs": model.structs,
        "api_classes": model.api_classes,
        "class_id_max": model.class_id_max,
        "value_slot_bytes": VALUE_SLOT_BYTES,
        "status_rows": status_rows(model),
        "proto_status_lines": proto_status_lines(model),
        "vedam_uuid_ns_c_array": ", ".join(f"0x{b:02x}" for b in VEDAM_UUID_NS.bytes),
        "vedam_uuid_ns_proto_esc": "".join(f"\\x{b:02x}" for b in VEDAM_UUID_NS.bytes),
        "wrap": wrap,
    }
