# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Load class and types YAML and build a template context."""

from __future__ import annotations

import pathlib
import uuid

import yaml

from .copyright import COPYRIGHT_C, GENERATED_C, GENERATED_PROTO
from .schema import load_schema, validate

# RFC 9562 UUID v5 namespace: UUID v5 of DNS name vedam.net.
VEDAM_UUID_NS = uuid.uuid5(uuid.NAMESPACE_DNS, "vedam.net")
VEDAM_UUID_NS_C = ", ".join(f"0x{b:02x}" for b in VEDAM_UUID_NS.bytes)

FLAG_C = {
    "MANDATORY_ON_CREATE": "VIA_F_MANDATORY_ON_CREATE",
    "CREATE_ONLY": "VIA_F_CREATE_ONLY",
    "CREATE_AND_SET": "VIA_F_CREATE_AND_SET",
    "READ_ONLY": "VIA_F_READ_ONLY",
    "SET_ONLY": "VIA_F_SET_ONLY",
    "KEY": "VIA_F_KEY",
    "SUBSCRIBABLE": "VIA_F_SUBSCRIBABLE",
}

TYPE_KIND = {
    "vi_node_class_t": "VIA_T_S32",
    "vi_system_node_class_t": "VIA_T_S32",
    "vint32_t": "VIA_T_S32",
    "v_version_t": "VIA_T_VERSION",
    "vi_system_state_t": "VIA_T_PTR",
    "vbool_t": "VIA_T_BOOL",
    "v_text_t": "VIA_T_TEXT",
    "v_transport_t": "VIA_T_TRANSPORT",
    "v_transport_ops_t": "VIA_T_TRANSPORT",
}


def load_yaml(path: pathlib.Path) -> dict:
    with path.open(encoding="utf-8") as fh:
        return yaml.safe_load(fh)


def flags_or(flags: list | None) -> str:
    if not flags:
        return "0"
    return " | ".join(FLAG_C[f] for f in flags)


def flags_doc(flags: list | None) -> str:
    return " | ".join(flags) if flags else "none"


def enrich_attr(name: str, spec: dict) -> dict:
    out = dict(spec)
    out["name"] = name
    out["flags_public"] = spec.get("flags_public") or []
    out["flags_internal"] = spec.get("flags_internal") or spec.get("flags_public") or []
    out["flags_public_doc"] = flags_doc(out["flags_public"])
    out["flags_internal_doc"] = flags_doc(out["flags_internal"])
    out["flags_public_c"] = flags_or(out["flags_public"])
    out["flags_internal_c"] = flags_or(out["flags_internal"])
    out["is_public"] = 1 if spec.get("visibility") == "public" else 0
    out["type_kind"] = TYPE_KIND.get(spec.get("type", ""), "VIA_T_NONE")
    return out


def proto_status_lines(types: dict) -> list[str]:
    lines = []
    for i, name in enumerate(types["proto_status"]):
        if name == "UNSPECIFIED":
            lines.append(f"  STATUS_CODE_UNSPECIFIED = {i};")
        elif name == "OK":
            lines.append(f"  STATUS_OK = {i};")
        else:
            lines.append(f"  STATUS_{name} = {i};")
    return lines


def status_rows(types: dict) -> list[dict]:
    items = types["status"]
    name_w = max(len(s["name"]) for s in items)
    val_w = max(len(str(s["value"])) for s in items)
    rows = []
    for s in items:
        rows.append(
            {
                "ident": f"V_STATUS_{s['name']}".ljust(len("V_STATUS_") + name_w),
                "value": str(s["value"]).rjust(val_w),
                "comment": s["comment"],
            }
        )
    return rows


def class_uuid_bytes(name: str) -> tuple[str, str, list[int]]:
    """Return (uuid string, C initializer, byte list) for a hashed name."""
    u = uuid.uuid5(VEDAM_UUID_NS, name)
    raw = list(u.bytes)
    c_array = ", ".join(f"0x{b:02x}" for b in raw)
    return str(u), c_array, raw


def prepare_uuid_lines(class_name: str, lines: list) -> list[dict]:
    """Build UUID v5 rows for vertical/major/minor/commit lines."""
    out = []
    for line in lines:
        major = line["major"]
        minor = line["minor"]
        commit = str(line["commit"])
        hashed = f"{class_name}/{major}/{minor}/{commit}"
        uuid_str, uuid_c, uuid_raw = class_uuid_bytes(hashed)
        out.append(
            {
                "major": major,
                "minor": minor,
                "commit": commit,
                "hashed_name": hashed,
                "uuid": uuid_str,
                "uuid_c_array": uuid_c,
                "uuid_bytes": uuid_raw,
            }
        )
    return out


def latest_uuid_line(lines: list[dict]) -> dict:
    """Return the uuid_lines row with the highest major, then minor."""
    return max(lines, key=lambda row: (row["major"], row["minor"]))


def proto_type_name(c_type: str) -> str:
    """Pascal-case a C type for a protobuf enum in that vertical's file."""
    name = c_type
    if name.startswith("vi_"):
        name = name[3:]
    if name.endswith("_t") or name.endswith("_e"):
        name = name[:-2]
    return "".join(part.capitalize() for part in name.split("_") if part)


def prepare_proto_enums(enums: list) -> list[dict]:
    """Enums from class YAML, named for the vertical proto file."""
    out = []
    for enum in enums:
        prefix = enum.get("prefix") or ""
        if prefix.startswith("VI_"):
            prefix = prefix[3:]
        values = []
        for value in enum.get("values") or []:
            values.append(
                {
                    "name": f"{prefix}{value['name']}",
                    "value": value["value"],
                    "comment": value.get("comment") or "",
                }
            )
        out.append({"name": proto_type_name(enum["c_type"]), "c_type": enum["c_type"], "values": values})
    return out


_PROTO_FIELD = {
    "s32": "value.s32",
    "u32": "value.u32",
    "u64": "value.u64",
    "version": "value.version",
    "text": "value.text",
    "bool": "value.booldata",
    "booldata": "value.booldata",
    "sysstate": "value.blob",
    "blob": "value.blob",
}

_FLAG_SENTENCE = {
    "READ_ONLY": "Do not send a new value.",
    "SUBSCRIBABLE": "Subscribe and Unsubscribe may name this id.",
    "CREATE_ONLY": "Only create may write this value.",
    "SET_ONLY": "Set may write this value.",
    "MANDATORY_ON_CREATE": "Create must include this id.",
    "KEY": "This id is the object key. Also set Message.key.",
    "CREATE_AND_SET": "Create and set may write this value.",
}


def _caller_comment(text: str) -> str:
    """Drop C pointer instructions from a comment shown to message callers."""
    kept = []
    for sentence in text.split("."):
        part = sentence.strip()
        if not part or "ptr" in part or "vi_" in part or "UUID" in part or part.startswith("Pass "):
            continue
        kept.append(part)
    if not kept:
        return ""
    return ". ".join(kept) + "."


def _wrap_note(text: str, width: int = 76) -> list[str]:
    words = text.split()
    lines = []
    current = ""
    for word in words:
        if not current:
            current = word
        elif len(current) + 1 + len(word) <= width:
            current = f"{current} {word}"
        else:
            lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def prepare_proto_notes(raw: dict) -> list[str]:
    """Comment lines for one vertical proto: call shape and verbs."""
    title = raw["class"].capitalize()
    notes = []
    chunks = [
        f"{title} is one vertical.",
        "A call is Message, from vedam/types.proto. The request and the response are that same message.",
    ]
    if raw.get("singleton"):
        chunks.append("Leave key empty. This vertical has one object on the node.")
    else:
        chunks.append("Set key to the object key. An empty key does not name an object.")
    chunks.extend(
        [
            "Put each attribute id in Attr.id, using the attribute enum in this file. A repeated id keeps the last entry.",
            "On Get, leave Attr.value empty. The response writes the value and Attr.status.",
            "Every id is visited. Message.status is success only when every Attr.status succeeded. Otherwise it is the first failure.",
            "",
            "Get fills the values for the ids in the request.",
            "Subscribe registers a watch for those ids and returns one Message. It does not stream.",
            "Notify is the stream of later changes for a watch registered by Subscribe. Only an id marked for subscribe is accepted.",
            "Unsubscribe stops watches for those ids. An empty attribute list stops every watch for this caller.",
            "",
            "The attribute enum below is the set of ids. A value does not change meaning, and it is not reused for a different attribute.",
        ]
    )
    for chunk in chunks:
        if chunk == "":
            notes.append("")
        else:
            notes.extend(_wrap_note(chunk))
    return notes


def prepare_proto_attr_enum(class_name: str, attrs: list[dict], enums: list[dict]) -> dict:
    """Public attribute ids as a protobuf enum for this vertical."""
    enum_by_type = {enum["c_type"]: enum["name"] for enum in enums}
    prefix = f"{class_name.upper()}_ATTR_"
    values = [
        {
            "name": f"{prefix}UNSPECIFIED",
            "value": 0,
            "comments": ["No attribute is selected."],
        }
    ]
    for attr in attrs:
        if not attr["is_public"]:
            continue
        field = _PROTO_FIELD.get(attr.get("proto_type") or "", "value.s32")
        wire = f"The response writes Attr.{field}."
        enum_name = enum_by_type.get(attr["type"])
        if enum_name and field == "value.s32":
            wire = f"The response writes Attr.{field}. The number is one of {enum_name}."
        elif attr.get("proto_type") == "sysstate":
            wire = "The response writes Attr.value.blob. Treat those bytes as opaque on this call."
        blurb = _caller_comment(attr.get("comment") or "")
        flags = [
            _FLAG_SENTENCE[flag] for flag in (attr.get("flags_public") or []) if flag in _FLAG_SENTENCE
        ]
        text = " ".join(part for part in (blurb, wire, " ".join(flags)) if part)
        values.append(
            {
                "name": f"{prefix}{attr['name']}",
                "value": attr["id"],
                "comments": _wrap_note(text),
            }
        )
    return {"name": f"{class_name.capitalize()}Attr", "values": values}


def prepare_class(raw: dict) -> dict:
    attrs = [enrich_attr(n, s) for n, s in raw["attrs"].items()]
    public_verbs = [name for name, spec in raw["verbs"].items() if spec.get("visibility") == "public"]
    uuid_lines = prepare_uuid_lines(raw["class"], raw["uuid_lines"])
    primary = latest_uuid_line(uuid_lines)
    major_max = max(line["major"] for line in uuid_lines)
    minor_max = max(line["minor"] for line in uuid_lines)
    uuid_grid = []
    by_mm = {(line["major"], line["minor"]): line for line in uuid_lines}
    for maj in range(0, major_max + 1):
        row = []
        for mn in range(0, minor_max + 1):
            hit = by_mm.get((maj, mn))
            row.append(hit["uuid_c_array"] if hit else "0")
        uuid_grid.append(row)
    proto_enums = prepare_proto_enums(raw.get("enums") or [])
    return {
        "raw": raw,
        "class_name": raw["class"],
        "class_ident": raw["class"].upper(),
        "proto_name": raw["class"].capitalize(),
        "verbs": raw["verbs"],
        "public_verbs": public_verbs,
        "enums": raw.get("enums") or [],
        "proto_enums": proto_enums,
        "proto_attr_enum": prepare_proto_attr_enum(raw["class"], attrs, proto_enums),
        "proto_notes": prepare_proto_notes(raw),
        "structs": raw.get("structs") or [],
        "attrs": attrs,
        "public_attrs": [a for a in attrs if a["is_public"]],
        "n_attrs": len(attrs),
        "singleton": bool(raw.get("singleton")),
        "uuid_lines": uuid_lines,
        "uuid": primary["uuid"],
        "uuid_c_array": primary["uuid_c_array"],
        "uuid_bytes": primary["uuid_bytes"],
        "uuid_proto_esc": "".join(f"\\x{b:02x}" for b in primary["uuid_bytes"]),
        "uuid_major": primary["major"],
        "uuid_minor": primary["minor"],
        "uuid_major_max": major_max,
        "uuid_minor_max": minor_max,
        "uuid_grid": uuid_grid,
        "has_ptr_attr": any(a["type_kind"] == "VIA_T_PTR" for a in attrs),
    }


def build_context(root: pathlib.Path) -> dict:
    src = root / "tools/codegen/src"
    schema = load_schema(root)
    types = load_yaml(src / "types.yaml")
    loaded = []
    for path in sorted((src / "classes").glob("*.yaml")):
        loaded.append((path, load_yaml(path)))
    validate(schema, types, loaded)
    classes = [prepare_class(raw) for _, raw in loaded]
    class_ids = [c["id"] for c in types["classes"]]
    return {
        "copyright_c": COPYRIGHT_C.rstrip("\n"),
        "generated_c": GENERATED_C.rstrip("\n"),
        "generated_proto": GENERATED_PROTO.rstrip("\n"),
        "types": types,
        "status_rows": status_rows(types),
        "api_classes": types["classes"],
        "classes": classes,
        "proto_status_lines": proto_status_lines(types),
        "class_id_max": max(class_ids) if class_ids else 0,
        "vedam_uuid_ns": str(VEDAM_UUID_NS),
        "vedam_uuid_ns_c_array": VEDAM_UUID_NS_C,
        "vedam_uuid_ns_proto_esc": "".join(f"\\x{b:02x}" for b in VEDAM_UUID_NS.bytes),
    }
