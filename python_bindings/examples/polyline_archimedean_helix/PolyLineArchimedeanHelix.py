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

import math

import numpy as np
import viskores
from viskores.cont import ColorTable, DataSetBuilderExplicitIterative
from viskores.filter.geometry_refinement import Tube
from viskores.rendering import Actor, Camera, CanvasRayTracer, MapperRayTracer, Scene, View3D


def archimedean_spiral_to_cartesian(point):
    radius, theta, z = point
    return (radius * math.cos(theta), radius * math.sin(theta), z)


def tube_that_spiral(radius, num_line_segments, num_sides):
    # The Archimedian spiral is defined by the equation r = a + b*theta.
    # To extend to a 3D curve, use z = t, theta = t, r = a + b t.
    builder = DataSetBuilderExplicitIterative()
    point_ids = []

    a = 0.2
    b = 0.8
    for i in range(num_line_segments):
        t = 4.0 * 3.1415926 * (i + 1) / num_line_segments
        spiral_sample = archimedean_spiral_to_cartesian((a + b * t, t, t))
        point_ids.append(builder.AddPoint(spiral_sample))
    builder.AddCell(viskores.CELL_SHAPE_POLY_LINE, point_ids)

    dataset = builder.Create()

    # Turn the polyline into a tube.
    tube_filter = Tube()
    tube_filter.SetCapping(True)
    tube_filter.SetNumberOfSides(num_sides)
    tube_filter.SetRadius(radius)
    tube_dataset = tube_filter.Execute(dataset)

    bounds = tube_dataset.GetCoordinateSystem().GetBounds()
    total_extent = (
        bounds[1] - bounds[0],
        bounds[3] - bounds[2],
        bounds[5] - bounds[4],
    )
    magnitude = math.sqrt(
        total_extent[0] * total_extent[0]
        + total_extent[1] * total_extent[1]
        + total_extent[2] * total_extent[2]
    )
    direction = (
        total_extent[0] / magnitude,
        total_extent[1] / magnitude,
        total_extent[2] / magnitude,
    )

    camera = Camera()
    camera.ResetToBounds(bounds)
    camera.SetLookAt(
        (
            direction[0] * (magnitude * 0.5),
            direction[1] * (magnitude * 0.5),
            direction[2] * (magnitude * 0.5),
        )
    )
    camera.SetViewUp((0.0, 1.0, 0.0))
    camera.SetClippingRange(1.0, 100.0)
    camera.SetFieldOfView(60.0)
    camera.SetPosition(
        (
            direction[0] * (magnitude * 2.0),
            direction[1] * (magnitude * 2.0),
            direction[2] * (magnitude * 2.0),
        )
    )
    color_table = ColorTable("inferno")

    # Add a scalar field describing the spiral radius along the tube.
    values = [0.0] * tube_dataset.GetNumberOfPoints()
    for i in range(1, len(values), num_sides):
        t = 4.0 * 3.1415926 * (i + 1) / num_sides
        spiral_radius = a + b * t
        for j in range(i, min(i + num_sides, len(values))):
            values[j] = spiral_radius
    values[-1] = values[-2]

    tube_dataset.AddPointField("Spiral Radius", np.asarray(values, dtype=np.float64))

    # Setup a camera and point it to towards the center of the input data.
    scene = Scene()
    scene.AddActor(Actor(tube_dataset, "Spiral Radius", color_table))
    mapper = MapperRayTracer()
    canvas = CanvasRayTracer(2048, 2048)
    background = (0.2, 0.2, 0.2, 1.0)
    view = View3D(scene, mapper, canvas, camera, background)
    view.Paint()
    # We can save the file as a .NetBPM:
    view.SaveAs(f"tube_output_{num_sides}_sides.pnm")
    # Or as a .png:
    view.SaveAs(f"tube_output_{num_sides}_sides.png")


def main():
    # Radius of the tube:
    radius = 0.5
    # How many segments is the tube decomposed into?
    num_line_segments = 100
    # As numSides->infty, the tube becomes perfectly cylindrical:
    tube_that_spiral(radius, num_line_segments, 50)
    # Setting numSides = 4 makes a square around the polyline:
    tube_that_spiral(radius, num_line_segments, 4)


if __name__ == "__main__":
    main()
