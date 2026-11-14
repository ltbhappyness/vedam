# Copyright (C) 2026 LTBH Vedam project
# SPDX-License-Identifier: AGPL-3.0-only
# A changed copy of Vedam is not called Vedam.
# An application that only calls Vedam keeps its own license. See LICENSE.
"""Render every template into memory, keyed by its path in the source tree."""

from __future__ import annotations

import pathlib

from jinja2 import Environment, FileSystemLoader, StrictUndefined

from .emit import EMIT, PER_CLASS, PER_PACKAGE


def make_env(template_dir: pathlib.Path) -> Environment:
    return Environment(
        loader=FileSystemLoader(str(template_dir)),
        undefined=StrictUndefined,
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True,
    )


def render_tree(env: Environment, ctx: dict) -> dict[str, str]:
    """Return every generated text file, keyed by its path relative to the root."""
    out = {}
    for tmpl, dest in EMIT:
        out[dest] = env.get_template(tmpl).render(ctx)
    for cls in ctx["classes"]:
        one = dict(ctx, cls=cls)
        for tmpl, pattern in PER_CLASS:
            out[pattern.format(name=cls.name)] = env.get_template(tmpl).render(one)
        for pkg in cls.packages:
            one_pkg = dict(one, pkg=pkg)
            for tmpl, pattern in PER_PACKAGE:
                dest = pattern.format(name=cls.name, major=pkg.major)
                out[dest] = env.get_template(tmpl).render(one_pkg)
    return out


def write_file(path: pathlib.Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
