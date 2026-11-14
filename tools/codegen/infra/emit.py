# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Map shared Jinja templates to source-tree paths.

One template serves every class. The generator loops classes and fills {name}.
A per-package template also fills {major}, once per major in uuid_lines.
"""

from __future__ import annotations

EMIT = [
    ("c/vi_types.h.j2", "include/vi/types.h"),
    ("c/vi_vedam.h.j2", "include/vi/vedam.h"),
    ("c/vii_types.h.j2", "include/vii/types.h"),
    ("c/vii_meta.h.j2", "include/vii/meta.h"),
    ("c/classes.c.j2", "src/via/classes.c"),
    ("c/class_uuid.h.j2", "include/vti/class_uuid.h"),
    ("c/class_uuid.c.j2", "src/vta/class_uuid.c"),
    ("cxx/struct_codec.cc.j2", "src/vta/struct_codec.cc"),
    ("proto/types.proto.j2", "proto/vedam/v1/types.proto"),
]

PER_CLASS = [
    ("c/class_vi.h.j2", "include/vi/class/{name}.h"),
    ("c/class_vii.h.j2", "include/vii/class/{name}.h"),
    ("c/class_va.c.j2", "src/va/{name}.c"),
]

PER_PACKAGE = [
    ("proto/class.proto.j2", "proto/vedam/{name}/v{major}/{name}.proto"),
]
