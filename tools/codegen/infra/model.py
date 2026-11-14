# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Build one validated model of the shared types and every class from model/."""

from __future__ import annotations

import pathlib
import uuid
from dataclasses import dataclass, field

from .schema import SchemaError, load_and_validate, proto_type_name

# RFC 9562 UUID v5 namespace: UUID v5 of DNS name vedam.net.
VEDAM_UUID_NS = uuid.uuid5(uuid.NAMESPACE_DNS, "vedam.net")


@dataclass
class Kind:
    name: str
    member: str
    proto_field: str
    proto_type: str
    integer: bool
    get_needs: str
    comment: str
    index: int

    @property
    def c_ident(self) -> str:
        return f"VII_T_{self.name}"


@dataclass
class Flag:
    name: str
    comment: str
    bit: int

    @property
    def c_ident(self) -> str:
        return f"VII_F_{self.name}"


@dataclass
class Verb:
    name: str
    index: int
    method: str
    writes_values: bool
    collapse: bool
    checks_list: bool
    public_doc: str

    @property
    def c_ident(self) -> str:
        return f"V_VERB_{self.name.upper()}"

    @property
    def doc_brief(self) -> str:
        return self.public_doc.strip().splitlines()[0]

    @property
    def doc_body(self) -> list[str]:
        return self.public_doc.strip().splitlines()[1:]


@dataclass
class EnumValue:
    name: str
    value: int
    comment: str
    c_name: str
    proto_name: str


@dataclass
class Field:
    type: str
    name: str
    count: int | None
    wire: bool
    comment: str
    number: int
    typedef: "TypeDef | None" = None

    @property
    def c_decl(self) -> str:
        suffix = f"[{self.count}]" if self.count else ""
        return f"{self.type} {self.name}{suffix}"

    @property
    def proto_type(self) -> str:
        return self.typedef.proto_type

    @property
    def proto_decl(self) -> str:
        repeated = "repeated " if self.count else ""
        return f"{repeated}{self.proto_type} {self.name} = {self.number};"


@dataclass
class TypeDef:
    name: str
    form: str
    kind: Kind
    visibility: str = "public"
    proto_name: str = ""
    prefix: str = ""
    values: list[EnumValue] = field(default_factory=list)
    fields: list[Field] = field(default_factory=list)
    struct_index: int = -1
    cxx_namespace: str = ""

    @property
    def wire_fields(self) -> list[Field]:
        return [f for f in self.fields if f.wire]

    @property
    def cxx_message(self) -> str:
        return f"{self.cxx_namespace}::{self.proto_name}"

    @property
    def proto_type(self) -> str:
        """The protobuf type a reader sees: the kind's type for a builtin, else the class name."""
        return self.kind.proto_type if self.form == "builtin" else self.proto_name

    @property
    def storage_c(self) -> str:
        """The C type of one stored value: vint32_t for an enum, else the type itself."""
        return "vint32_t" if self.form == "enum" else self.name


@dataclass
class Attr:
    name: str
    id: int
    typedef: TypeDef
    visibility: str
    flags_public: list[str]
    flags_internal: list[str]
    comment: str
    range: tuple[int, int] | None
    max_len: int
    flag_by_name: dict[str, Flag]
    max_count: int = 0
    list_kind: Kind | None = None

    @property
    def is_list(self) -> bool:
        return self.max_count > 0

    @property
    def kind(self) -> Kind:
        return self.list_kind if self.is_list else self.typedef.kind

    @property
    def elem_kind(self) -> Kind:
        """The kind of one item for a list, or the attribute's own kind."""
        return self.typedef.kind

    @property
    def c_type(self) -> str:
        return self.typedef.name

    @property
    def size_expr(self) -> str:
        """Bytes of one struct value or one struct item, otherwise 0."""
        return f"sizeof({self.c_type})" if self.typedef.form == "struct" else "0"

    @property
    def elem_size_expr(self) -> str:
        """Bytes of one list item, otherwise 0."""
        return f"sizeof({self.typedef.storage_c})" if self.is_list else "0"

    @property
    def is_public(self) -> bool:
        return self.visibility == "public"

    @property
    def flags(self) -> list[str]:
        return self.flags_internal

    @property
    def flags_c(self) -> str:
        if not self.flags_internal:
            return "0"
        return " | ".join(self.flag_by_name[f].c_ident for f in self.flags_internal)

    @property
    def enum_type(self) -> TypeDef | None:
        return self.typedef if self.typedef.form == "enum" else None

    @property
    def struct_index(self) -> int:
        return self.typedef.struct_index if self.typedef.form == "struct" else 0

    @property
    def rules(self) -> list[tuple[str, str]]:
        """AttrRules fields for the protobuf option, only those that are set."""
        out = [("kind", f'"{self.kind.name}"'), ("type", f'"{self.typedef.proto_type}"')]
        if self.range is not None:
            out += [("has_range", "true"), ("min", str(self.range[0])), ("max", str(self.range[1]))]
        if self.max_len:
            out.append(("max_len", str(self.max_len)))
        out += [("flags", f'"{flag}"') for flag in self.flags_public]
        return out

    def docs(self, audience: str) -> list[tuple[str, str]] | str:
        """Every sentence about this attribute: type, kind, range, values, length, flags.

        public and internal return (tag, text) pairs for a C comment. proto returns
        the comment text for the protobuf enum value.
        """
        if audience == "proto":
            return self._proto_docs()
        flags = self.flags_public if audience == "public" else self.flags_internal
        out = [("type", f"a list of {self.c_type}" if self.is_list else self.c_type)]
        if audience == "internal":
            out.append(("kind", self.kind.name))
        if self.range is not None:
            out.append(("range", f"{self.range[0]} to {self.range[1]}"))
        if self.enum_type is not None:
            out.append(("values", self.enum_type.name))
        if self.max_len:
            out.append(("maxlen", f"{self.max_len} bytes"))
        if self.is_list:
            out.append(("maxcount", f"{self.max_count} items"))
            out.append(("get", "Set count to the room in items. On overflow count holds the needed number."))
        elif self.kind.name == "TEXT":
            out.append(("get", "Set len to the room in buf. On overflow len holds the needed length."))
        if flags:
            out.append(("flags", " | ".join(flags)))
        return out

    def _proto_docs(self) -> str:
        parts = []
        blurb = _caller_comment(self.comment)
        if blurb:
            parts.append(blurb)
        if self.is_list:
            parts.append(
                f"The response writes Attr.value.{self.kind.proto_field}, a list of "
                f"{self.typedef.proto_type}, at most {self.max_count} items."
            )
        elif self.kind.name == "STRUCT":
            parts.append(
                f"The response writes Attr.value.{self.kind.proto_field}, a google.protobuf.Any "
                f"that holds {self.typedef.proto_name}."
            )
        else:
            parts.append(f"The response writes Attr.value.{self.kind.proto_field}.")
        if self.enum_type is not None:
            parts.append(f"The number is one of {self.enum_type.proto_name}.")
        if self.range is not None:
            parts.append(f"A written value is from {self.range[0]} to {self.range[1]}.")
        if self.max_len:
            parts.append(f"A written text is at most {self.max_len} bytes.")
        parts.extend(self.flag_by_name[f].comment for f in self.flags_public)
        return " ".join(parts)


@dataclass
class ClassVerb:
    verb: Verb
    visibility: str
    stream: bool
    comment: str

    @property
    def is_public(self) -> bool:
        return self.visibility == "public"


@dataclass
class UuidLine:
    major: int
    minor: int
    commit: str
    c_array: str
    proto_esc: str


@dataclass
class Package:
    major: int
    minor: int
    name: str
    service: str
    full_service: str
    uuid: UuidLine
    path: str


@dataclass
class VClass:
    name: str
    ident: str
    id: int
    singleton: bool
    datastore: str
    verbs: list[ClassVerb]
    enums: list[TypeDef]
    structs: list[TypeDef]
    attrs: list[Attr]
    uuid_lines: list[UuidLine]
    packages: list[Package]
    latest: UuidLine
    uuid_grid: list[list[str]]

    @property
    def title(self) -> str:
        return self.name.capitalize()

    @property
    def public_verbs(self) -> list[ClassVerb]:
        return [v for v in self.verbs if v.is_public]

    @property
    def rpc_verbs(self) -> list[ClassVerb]:
        return [v for v in self.verbs if v.is_public or v.stream]

    @property
    def public_attrs(self) -> list[Attr]:
        return [a for a in self.attrs if a.is_public]

    @property
    def attr_enums(self) -> list[TypeDef]:
        used = {a.c_type for a in self.attrs}
        return [e for e in self.enums if e.name in used]

    @property
    def external_verbs_c(self) -> str:
        bits = [f"(1u << {v.verb.c_ident})" for v in self.public_verbs]
        return " | ".join(bits) if bits else "0"

    @property
    def attr_id_max(self) -> int:
        return max(a.id for a in self.attrs)

    @property
    def attr_by_id(self) -> list[int]:
        rows = [0] * (self.attr_id_max + 1)
        for i, attr in enumerate(self.attrs):
            rows[attr.id] = i + 1
        return rows

    @property
    def major_max(self) -> int:
        return max(line.major for line in self.uuid_lines)

    @property
    def minor_max(self) -> int:
        return max(line.minor for line in self.uuid_lines)

    @property
    def proto_notes(self) -> list[str]:
        return _proto_notes(self)


@dataclass
class Model:
    package: str
    kinds: list[Kind]
    builtin: dict[str, TypeDef]
    flags: list[Flag]
    verbs: list[Verb]
    status: list[dict]
    proto_status: list[str]
    api_classes: list[dict]
    classes: list[VClass]
    structs: list[TypeDef]

    @property
    def class_id_max(self) -> int:
        return max(c["id"] for c in self.api_classes)


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


def wrap(text: str, width: int = 76) -> list[str]:
    """Split @p text into lines no wider than @p width."""
    lines = []
    current = ""
    for word in text.split():
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


def _proto_notes(cls: VClass) -> list[str]:
    chunks = [
        f"{cls.title} is one vertical.",
        "A call is vedam.v1.Message. The request and the response are that same message.",
    ]
    if cls.singleton:
        chunks.append("Leave key empty. This vertical has one object on the node.")
    else:
        chunks.append("Set key to the object key. An empty key does not name an object.")
    chunks += [
        "Put each attribute id in Attr.id, using the attribute enum in this file. "
        "A repeated id keeps the last entry.",
        "On Get, leave Attr.value empty. The response writes the value and Attr.status.",
        "Every id is visited. Message.status is success only when every Attr.status succeeded. "
        "Otherwise it is the first failure.",
        "",
        "The attribute enum below is the set of ids. A value does not change meaning, "
        "and it is not reused for a different attribute. The option on each value "
        "states its kind, type, and limits.",
    ]
    notes: list[str] = []
    for chunk in chunks:
        if chunk:
            notes.extend(wrap(chunk))
        else:
            notes.append("")
    return notes


def _uuid_line(class_name: str, line: dict) -> UuidLine:
    commit = str(line["commit"])
    u = uuid.uuid5(VEDAM_UUID_NS, f"{class_name}/{line['major']}/{line['minor']}/{commit}")
    raw = u.bytes
    return UuidLine(
        major=line["major"],
        minor=line["minor"],
        commit=commit,
        c_array=", ".join(f"0x{b:02x}" for b in raw),
        proto_esc="".join(f"\\x{b:02x}" for b in raw),
    )


def _enum(raw: dict, kind: Kind, namespace: str) -> TypeDef:
    prefix = raw["prefix"]
    proto_prefix = prefix[3:] if prefix.startswith("VI_") else prefix
    values = [
        EnumValue(v["name"], v["value"], v["comment"], f"{prefix}{v['name']}", f"{proto_prefix}{v['name']}")
        for v in raw["values"]
    ]
    return TypeDef(
        name=raw["c_type"],
        form="enum",
        kind=kind,
        visibility=raw.get("visibility", "public"),
        proto_name=raw.get("proto_name") or proto_type_name(raw["c_type"]),
        prefix=prefix,
        values=values,
        cxx_namespace=namespace,
    )


def _struct(raw: dict, kind: Kind, namespace: str, index: int) -> TypeDef:
    fields = [
        Field(
            type=f["type"],
            name=f["name"],
            count=f.get("count"),
            wire=f.get("wire", True),
            comment=f["comment"],
            number=i + 1,
        )
        for i, f in enumerate(raw["fields"])
    ]
    return TypeDef(
        name=raw["c_type"],
        form="struct",
        kind=kind,
        visibility=raw.get("visibility", "public"),
        proto_name=raw.get("proto_name") or proto_type_name(raw["c_type"]),
        fields=fields,
        struct_index=index,
        cxx_namespace=namespace,
    )


def _resolve_fields(struct: TypeDef, types: dict[str, TypeDef]) -> None:
    for f in struct.fields:
        f.typedef = types[f.type]
        if not f.wire:
            continue
        if f.typedef.form == "struct" or f.typedef.kind.get_needs != "none":
            raise SchemaError(
                f"{struct.name}.{f.name}: a {f.typedef.kind.name} member cannot travel on the wire; "
                "set wire: false"
            )


def _packages(model_pkg: str, name: str, lines: list[UuidLine]) -> list[Package]:
    out = []
    for major in sorted({line.major for line in lines}):
        latest = max((line for line in lines if line.major == major), key=lambda line: line.minor)
        pkg = f"{model_pkg}.{name}.v{major}"
        service = f"{name.capitalize()}Service"
        out.append(
            Package(
                major=major,
                minor=latest.minor,
                name=pkg,
                service=service,
                full_service=f"{pkg}.{service}",
                uuid=latest,
                path=f"proto/{model_pkg}/{name}/v{major}/{name}.proto",
            )
        )
    return out


def _build_class(raw: dict, ctx: dict) -> VClass:
    name = raw["class"]
    lines = [_uuid_line(name, line) for line in raw["uuid_lines"]]
    latest = max(lines, key=lambda line: (line.major, line.minor))
    namespace = f"{ctx['package']}::{name}::v{latest.major}"
    types = dict(ctx["builtin"])
    enums = [_enum(e, ctx["kinds"]["S32"], namespace) for e in raw.get("enums") or []]
    structs = []
    for s in raw.get("structs") or []:
        structs.append(_struct(s, ctx["kinds"]["STRUCT"], namespace, len(ctx["structs"])))
        ctx["structs"].append(structs[-1])
    for td in enums + structs:
        types[td.name] = td
    for td in structs:
        _resolve_fields(td, types)
    attrs = []
    for attr_name, spec in raw["attrs"].items():
        rng = spec.get("range")
        attrs.append(
            Attr(
                name=attr_name,
                id=spec["id"],
                typedef=types[spec["type"]],
                visibility=spec["visibility"],
                flags_public=list(spec.get("flags_public") or []),
                flags_internal=list(spec.get("flags_internal") or spec.get("flags_public") or []),
                comment=spec["comment"],
                range=(rng["min"], rng["max"]) if rng else None,
                max_len=spec.get("max_len", 0),
                flag_by_name=ctx["flag_by_name"],
                max_count=spec["list"]["max_count"] if "list" in spec else 0,
                list_kind=ctx["kinds"]["LIST"],
            )
        )
    verbs = []
    for verb in ctx["verbs"]:
        spec = raw["verbs"][verb.name]
        verbs.append(ClassVerb(verb, spec["visibility"], bool(spec.get("stream")), spec["comment"]))
    by_mm = {(line.major, line.minor): line for line in lines}
    major_max = max(line.major for line in lines)
    minor_max = max(line.minor for line in lines)
    grid = [
        [by_mm[(maj, mn)].c_array if (maj, mn) in by_mm else "0" for mn in range(minor_max + 1)]
        for maj in range(major_max + 1)
    ]
    return VClass(
        name=name,
        ident=name.upper(),
        id=ctx["class_id"][name.upper()],
        singleton=raw["singleton"],
        datastore=raw["datastore"],
        verbs=verbs,
        enums=enums,
        structs=structs,
        attrs=attrs,
        uuid_lines=lines,
        packages=_packages(ctx["package"], name, lines),
        latest=latest,
        uuid_grid=grid,
    )


def load_model(root: pathlib.Path) -> Model:
    """Validate model/ and return the model every template reads."""
    types, loaded = load_and_validate(root)
    kinds = [Kind(index=i, **k) for i, k in enumerate(types["kinds"])]
    kind_by_name = {k.name: k for k in kinds}
    builtin = {
        name: TypeDef(name=name, form="builtin", kind=kind_by_name[kind])
        for name, kind in types["builtin_types"].items()
    }
    flags = [Flag(f["name"], f["comment"], i) for i, f in enumerate(types["attr_flags"])]
    verbs = [
        Verb(
            name=v["name"],
            index=i,
            method=v["method"],
            writes_values=v["writes_values"],
            collapse=v["collapse"],
            checks_list=v["checks_list"],
            public_doc=v["public_doc"],
        )
        for i, v in enumerate(types["verbs"])
    ]
    ctx = {
        "package": types["package"],
        "kinds": kind_by_name,
        "builtin": builtin,
        "flag_by_name": {f.name: f for f in flags},
        "verbs": verbs,
        "structs": [],
        "class_id": {c["name"]: c["id"] for c in types["classes"]},
    }
    classes = [_build_class(raw, ctx) for _, raw in loaded]
    return Model(
        package=types["package"],
        kinds=kinds,
        builtin=builtin,
        flags=flags,
        verbs=verbs,
        status=types["status"],
        proto_status=types["proto_status"],
        api_classes=types["classes"],
        classes=classes,
        structs=ctx["structs"],
    )
