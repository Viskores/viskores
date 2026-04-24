#!/usr/bin/env python3

##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import importlib.util
import sys
import types
from pathlib import Path

import numpy as np


def load_example_module():
    example = (
        Path(__file__).resolve().parent.parent / "examples" / "game_of_life" / "GameOfLife.py"
    )
    spec = importlib.util.spec_from_file_location("viskores_game_of_life_demo", example)
    module = importlib.util.module_from_spec(spec)
    fake_pyglet = types.ModuleType("pyglet")
    fake_pyglet.app = types.SimpleNamespace(run=lambda: None, exit=lambda: None)
    fake_pyglet.clock = types.SimpleNamespace(
        schedule_interval=lambda *args, **kwargs: None,
        unschedule=lambda *args, **kwargs: None,
    )
    fake_pyglet.window = types.SimpleNamespace(
        key=types.SimpleNamespace(ESCAPE="ESCAPE"),
        Window=object,
    )
    fake_opengl = types.ModuleType("OpenGL")
    fake_gl = types.ModuleType("OpenGL.GL")
    fake_opengl.GL = fake_gl
    old_pyglet = sys.modules.get("pyglet")
    old_opengl = sys.modules.get("OpenGL")
    old_opengl_gl = sys.modules.get("OpenGL.GL")
    sys.modules["pyglet"] = fake_pyglet
    sys.modules["OpenGL"] = fake_opengl
    sys.modules["OpenGL.GL"] = fake_gl
    assert spec.loader is not None
    try:
        spec.loader.exec_module(module)
    finally:
        if old_pyglet is None:
            sys.modules.pop("pyglet", None)
        else:
            sys.modules["pyglet"] = old_pyglet
        if old_opengl is None:
            sys.modules.pop("OpenGL", None)
        else:
            sys.modules["OpenGL"] = old_opengl
        if old_opengl_gl is None:
            sys.modules.pop("OpenGL.GL", None)
        else:
            sys.modules["OpenGL.GL"] = old_opengl_gl
    return module


def main():
    demo = load_example_module()
    state = demo.populate(96, 96, 0.275)
    colors = demo.colors_from_state(state, np.zeros_like(state), np.zeros_like(state))
    dataset = demo.build_dataset(state, colors)
    game = demo.GameOfLife(96, 96)

    current = dataset
    for _ in range(4):
        current = game.Execute(current)

    final_state = current.GetField("state")
    final_colors = current.GetField("colors")

    assert final_state.shape == (96 * 96,)
    assert final_colors.shape == (96 * 96, 4)
    assert final_colors.dtype == np.uint8
    assert int(final_state.sum()) > 0


if __name__ == "__main__":
    main()
