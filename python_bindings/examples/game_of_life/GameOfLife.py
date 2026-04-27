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

import argparse
import sys
import time

import numpy as np
import pyglet
from OpenGL import GL

import viskores
import viskores.cont
from viskores.cont import Association
from viskores.interop import BufferState, TransferToOpenGL


def make_fragment_shader_code():
    return """#version 150
in vec4 fragColor;
out vec4 outputColor;
void main()
{
    outputColor = fragColor;
}
"""


def make_vertex_shader_code():
    return """#version 150
in vec3 posAttr;
in vec4 colorAttr;
out vec4 fragColor;
uniform mat4 MVP;
void main()
{
    fragColor = colorAttr;
    gl_Position = MVP * vec4(posAttr, 1.0);
}
"""


def compile_shader(shader_type, shader_code):
    shader_id = GL.glCreateShader(shader_type)
    GL.glShaderSource(shader_id, shader_code)
    GL.glCompileShader(shader_id)
    result = GL.glGetShaderiv(shader_id, GL.GL_COMPILE_STATUS)
    if not result:
        message = GL.glGetShaderInfoLog(shader_id)
        raise RuntimeError(f"OpenGL shader compilation failed: {message}")
    return shader_id


def load_shaders():
    vertex_shader = compile_shader(GL.GL_VERTEX_SHADER, make_vertex_shader_code())
    fragment_shader = compile_shader(GL.GL_FRAGMENT_SHADER, make_fragment_shader_code())

    program = GL.glCreateProgram()
    GL.glAttachShader(program, vertex_shader)
    GL.glAttachShader(program, fragment_shader)
    GL.glBindAttribLocation(program, 0, b"posAttr")
    GL.glBindAttribLocation(program, 1, b"colorAttr")
    GL.glLinkProgram(program)

    result = GL.glGetProgramiv(program, GL.GL_LINK_STATUS)
    if not result:
        message = GL.glGetProgramInfoLog(program)
        raise RuntimeError(f"OpenGL program link failed: {message}")

    GL.glDeleteShader(vertex_shader)
    GL.glDeleteShader(fragment_shader)
    return program


def build_coordinates(width, height):
    # Generate coordinates and render them.
    origin = np.asarray((-4.0, -4.0, 0.0), dtype=np.float32)
    spacing = np.asarray((0.0075, 0.0075, 0.0), dtype=np.float32)
    x_coords = origin[0] + (spacing[0] * np.arange(width, dtype=np.float32))
    y_coords = origin[1] + (spacing[1] * np.arange(height, dtype=np.float32))
    xx, yy = np.meshgrid(x_coords, y_coords, indexing="xy")
    zz = np.zeros_like(xx, dtype=np.float32)
    return np.stack((xx, yy, zz), axis=-1).reshape(-1, 3)


def stamp_acorn(state, x, y):
    acorn = np.asarray(
        [
            [0, 0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 1, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 1, 0, 0, 0, 0],
            [0, 1, 1, 0, 0, 1, 1, 1, 0],
            [0, 0, 0, 0, 0, 0, 0, 0, 0],
        ],
        dtype=np.uint8,
    )
    height, width = acorn.shape
    state[y : y + height, x : x + width] = acorn


def populate(width, height, rate):
    # Populate the initial state.
    rng = np.random.default_rng()
    state = rng.binomial(1, rate, size=(height, width)).astype(np.uint8)

    for x in range(2, max(2, width - 64), 64):
        y = 2
        while y < max(2, height - 64):
            if (x + 9) < width and (y + 5) < height:
                stamp_acorn(state, x, y)
            y += 64

    return state


def colors_from_state(state, previous_state, neighbor_count):
    colors = np.zeros(state.shape + (4,), dtype=np.uint8)
    colors[..., 1] = (state * np.minimum(100 + (neighbor_count * 32), 255)).astype(np.uint8)
    colors[..., 2] = np.where((state == 1) & (previous_state == 0), np.minimum(100 + (neighbor_count * 32), 255), 0)
    colors[..., 3] = 255
    return colors


def build_dataset(state, colors):
    # Save the results.
    dataset = viskores.create_uniform_dataset((int(state.shape[1]), int(state.shape[0])))
    dataset.AddPointField("state", state.reshape(-1))
    dataset.AddPointField("colors", colors.reshape(-1, 4))
    return dataset


class GameOfLife:
    def __init__(self, width, height):
        self.width = width
        self.height = height

    def Execute(self, dataset):
        # Get the previous state of the game.
        previous_state = dataset.GetField("state").reshape((self.height, self.width)).astype(np.uint8)
        padded = np.pad(previous_state, 1, mode="constant")
        neighbor_count = (
            padded[:-2, :-2]
            + padded[:-2, 1:-1]
            + padded[:-2, 2:]
            + padded[1:-1, :-2]
            + padded[1:-1, 2:]
            + padded[2:, :-2]
            + padded[2:, 1:-1]
            + padded[2:, 2:]
        )

        # Update the game state.
        state = np.where(
            ((previous_state == 1) & ((neighbor_count == 2) | (neighbor_count == 3)))
            | ((previous_state == 0) & (neighbor_count == 3)),
            1,
            0,
        ).astype(np.uint8)

        colors = colors_from_state(state, previous_state, neighbor_count)
        # Save the results.
        return build_dataset(state, colors)


class RenderGameOfLife:
    def __init__(self, screen_width, screen_height, width, height, cell_size):
        # Generate coords and render them.
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.width = width
        self.height = height
        self.cell_size = cell_size
        self.shader_program_id = load_shaders()
        self.vao_id = GL.glGenVertexArrays(1)
        GL.glUseProgram(self.shader_program_id)
        GL.glBindVertexArray(self.vao_id)
        GL.glClearColor(0.0, 0.0, 0.0, 0.0)
        GL.glPointSize(float(max(1, cell_size)))
        GL.glViewport(0, 0, self.screen_width, self.screen_height)
        self.vbo_state = BufferState(buffer_type=GL.GL_ARRAY_BUFFER)
        self.color_state = BufferState(buffer_type=GL.GL_ARRAY_BUFFER)
        TransferToOpenGL(build_coordinates(width, height), self.vbo_state)
        self.mvp = np.asarray(
            [
                1.0,
                0.0,
                0.0,
                0.0,
                0.0,
                1.0,
                0.0,
                0.0,
                0.0,
                0.0,
                1.0,
                0.0,
                0.0,
                0.0,
                0.0,
                3.5,
            ],
            dtype=np.float32,
        )

    def render(self, dataset, step):
        # Render the current game state.
        GL.glBindVertexArray(self.vao_id)
        GL.glUseProgram(self.shader_program_id)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
        TransferToOpenGL(
            dataset,
            self.color_state,
            field_name="colors",
            association=Association.POINTS,
        )

        uniform_location = GL.glGetUniformLocation(self.shader_program_id, "MVP")
        GL.glUniformMatrix4fv(uniform_location, 1, GL.GL_FALSE, self.mvp)

        GL.glEnableVertexAttribArray(0)
        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self.vbo_state.GetHandle())
        GL.glVertexAttribPointer(0, 3, GL.GL_FLOAT, GL.GL_FALSE, 0, None)

        GL.glEnableVertexAttribArray(1)
        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self.color_state.GetHandle())
        GL.glVertexAttribPointer(1, 4, GL.GL_UNSIGNED_BYTE, GL.GL_TRUE, 0, None)

        GL.glDrawArrays(GL.GL_POINTS, 0, dataset.GetNumberOfPoints())

        GL.glDisableVertexAttribArray(1)
        GL.glDisableVertexAttribArray(0)


class GameOfLifeWindow:
    def __init__(
        self,
        filter_obj,
        dataset,
        screen_width,
        screen_height,
        grid_width,
        grid_height,
        max_runtime_seconds,
    ):
        self.filter = filter_obj
        self.dataset = dataset
        self.max_runtime_seconds = max_runtime_seconds
        self.step = 0
        self.start_time = time.monotonic()
        self.app = pyglet.app
        self.clock = pyglet.clock
        self.key = pyglet.window.key
        config = pyglet.gl.Config(
            major_version=3,
            minor_version=2,
            forward_compatible=True,
            double_buffer=True,
        )
        self.window = pyglet.window.Window(
            width=screen_width,
            height=screen_height,
            caption="Viskores Game Of Life",
            resizable=False,
            config=config,
        )
        self.renderer = RenderGameOfLife(
            screen_width,
            screen_height,
            grid_width,
            grid_height,
            1,
        )
        self.window.push_handlers(self)
        self.clock.schedule_interval(self.tick, 1.0 / 60.0)

    def on_draw(self):
        self.renderer.render(self.dataset, self.step)
        self.window.invalid = False

    def on_key_press(self, symbol, modifiers):
        if symbol == self.key.ESCAPE:
            self.close()

    def tick(self, _delta_time):
        if (time.monotonic() - self.start_time) > self.max_runtime_seconds:
            self.close()
            return
        self.dataset = self.filter.Execute(self.dataset)
        self.step += 1
        self.window.invalid = True

    def close(self):
        self.clock.unschedule(self.tick)
        self.window.close()
        self.app.exit()

    def run(self):
        self.app.run()


def main():
    screen_width = 1024
    screen_height = 768
    grid_width = 1024
    grid_height = 1024
    default_rate = 0.275
    max_runtime_seconds = 120.0

    parser = argparse.ArgumentParser(description="Python port of examples/game_of_life/GameOfLife.cxx")
    parser.add_argument("rate", nargs="?", type=float, default=default_rate)
    args, remaining_argv = parser.parse_known_args()

    # Initialize Viskores.
    viskores.cont.Initialize(
        [sys.argv[0], *remaining_argv],
        viskores.cont.InitializeOptions.DefaultAnyDevice | viskores.cont.InitializeOptions.Strict,
    )

    # Create the initial game state.
    rate = max(0.0001, min(0.9, args.rate))
    state = populate(grid_width, grid_height, rate)
    colors = colors_from_state(state, np.zeros_like(state), np.zeros_like(state))
    dataset = build_dataset(state, colors)

    # Run the OpenGL application.
    filter_obj = GameOfLife(grid_width, grid_height)
    try:
        window = GameOfLifeWindow(
            filter_obj,
            dataset,
            screen_width,
            screen_height,
            grid_width,
            grid_height,
            max_runtime_seconds,
        )
        window.run()
    except Exception as error:
        print(f"windowed mode failed: {error}", file=sys.stderr)
        raise


if __name__ == "__main__":
    main()
