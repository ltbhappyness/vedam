# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
from .context import build_context
from .copyright import COPYRIGHT_C, GENERATED_C
from .emit import EMIT
from .render import make_env, render, write_file

__all__ = [
    "COPYRIGHT_C",
    "EMIT",
    "GENERATED_C",
    "build_context",
    "make_env",
    "render",
    "write_file",
]

