# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Map shared Jinja templates to source-tree paths.

One template serves every class. The generator loops classes and fills {class}.
"""

from __future__ import annotations

EMIT = [
    ("proto/types.proto.j2", "proto/vedam/types.proto"),
    ("c/vi_types.h.j2", "include/vi/types.h"),
    ("c/vi_vedam.h.j2", "include/vi/vedam.h"),
    ("c/vii_types.h.j2", "include/vii/types.h"),
    ("c/via_meta.h.j2", "src/via/meta.h"),
    ("c/dispatch.c.j2", "src/vta/dispatch.c"),
    ("c/attr_ids.c.j2", "src/lock/attr_ids.c"),
    ("c/attrwire.c.j2", "src/vta/attrwire.c"),
    ("c/class_uuid.h.j2", "include/vti/class_uuid.h"),
    ("c/class_uuid.c.j2", "src/vta/class_uuid.c"),
    ("cxx/codec.cc.j2", "src/vta/codec.cc"),
]

PER_CLASS = [
    ("c/class_vi.h.j2", "include/vi/class/{name}.h"),
    ("c/class_vii.h.j2", "include/vii/class/{name}.h"),
    ("c/class_va.c.j2", "src/va/{name}.c"),
    ("c/class_via.c.j2", "src/via/{name}.c"),
    ("proto/class.proto.j2", "proto/vedam/{name}.proto"),
]
