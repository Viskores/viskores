##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from enum import IntEnum

from ._viskores import (
    ASSOCIATION_ANY,
    ASSOCIATION_CELLS,
    ASSOCIATION_GLOBAL,
    ASSOCIATION_PARTITIONS,
    ASSOCIATION_POINTS,
    ASSOCIATION_WHOLE_DATASET,
    DataSet,
    PartitionedDataSet,
    cell_average,
    contour,
    contour_tree_augmented,
    contour_tree_distributed,
    create_tangle_dataset,
    create_uniform_dataset,
    distributed_branch_decomposition,
    extract_top_volume_contours,
    field_range,
    gradient,
    partition_uniform_dataset,
    point_average,
    render_dataset,
    select_top_volume_branches,
    vector_magnitude,
)


class Association(IntEnum):
    ANY = ASSOCIATION_ANY
    WHOLE_DATASET = ASSOCIATION_WHOLE_DATASET
    POINTS = ASSOCIATION_POINTS
    CELLS = ASSOCIATION_CELLS
    PARTITIONS = ASSOCIATION_PARTITIONS
    GLOBAL = ASSOCIATION_GLOBAL


class Tangle:
    def __init__(self):
        self._point_dimensions = None

    def set_point_dimensions(self, dims):
        self._point_dimensions = tuple(dims)

    def execute(self):
        if self._point_dimensions is None:
            return create_tangle_dataset()
        return create_tangle_dataset(self._point_dimensions)


class Contour:
    def __init__(self):
        self._iso_values = []
        self._active_field = None
        self._generate_normals = False

    def set_iso_value(self, value):
        self._iso_values = [float(value)]

    def set_active_field(self, name):
        self._active_field = name

    def set_generate_normals(self, enabled):
        self._generate_normals = bool(enabled)

    def execute(self, dataset):
        if self._active_field is None:
            raise RuntimeError("Active field must be set before executing the contour filter.")
        if not self._iso_values:
            raise RuntimeError("At least one iso value must be set before executing the contour filter.")
        return contour(
            dataset,
            self._active_field,
            self._iso_values,
            generate_normals=self._generate_normals,
        )


class Camera:
    def __init__(self):
        self.look_at = (0.5, 0.5, 0.5)
        self.view_up = (0.0, 1.0, 0.0)
        self.clipping_range = (1.0, 10.0)
        self.field_of_view = 60.0
        self.position = (1.5, 1.5, 1.5)

    def set_look_at(self, value):
        self.look_at = tuple(value)

    def set_view_up(self, value):
        self.view_up = tuple(value)

    def set_clipping_range(self, near_plane, far_plane):
        self.clipping_range = (float(near_plane), float(far_plane))

    def set_field_of_view(self, value):
        self.field_of_view = float(value)

    def set_position(self, value):
        self.position = tuple(value)


class ColorTable:
    def __init__(self, name):
        self.name = name


class Actor:
    def __init__(self, dataset, field_name, color_table):
        self.dataset = dataset
        self.field_name = field_name
        self.color_table = color_table
        self._scalar_range = None

    def get_scalar_range(self):
        if self._scalar_range is None:
            self._scalar_range = tuple(field_range(self.dataset, self.field_name))
        return self._scalar_range

    def set_scalar_range(self, scalar_range):
        self._scalar_range = tuple(scalar_range)


class Scene:
    def __init__(self):
        self.actors = []

    def add_actor(self, actor):
        self.actors.append(actor)


class CanvasRayTracer:
    def __init__(self, width=1024, height=1024):
        self.width = int(width)
        self.height = int(height)


class MapperVolume:
    name = "volume"


class MapperWireframer:
    name = "wireframe"


class MapperRayTracer:
    name = "raytracer"


class View3D:
    def __init__(self, scene, mapper, canvas, camera, background):
        self.scene = scene
        self.mapper = mapper
        self.canvas = canvas
        self.camera = camera
        self.background = tuple(background)
        self._painted = False

    def paint(self):
        self._painted = True

    def save_as(self, filename):
        if len(self.scene.actors) != 1:
            raise RuntimeError("The current Python View3D wrapper supports exactly one actor per scene.")
        actor = self.scene.actors[0]
        render_dataset(
            actor.dataset,
            actor.field_name,
            filename,
            mapper=self.mapper.name,
            image_size=(self.canvas.width, self.canvas.height),
            camera_look_at=self.camera.look_at,
            camera_view_up=self.camera.view_up,
            camera_position=self.camera.position,
            camera_clipping_range=self.camera.clipping_range,
            camera_field_of_view=self.camera.field_of_view,
            background=self.background,
            color_table=actor.color_table.name,
            scalar_range=actor.get_scalar_range(),
        )


__all__ = [
    "Actor",
    "Association",
    "Camera",
    "CanvasRayTracer",
    "ColorTable",
    "Contour",
    "DataSet",
    "MapperRayTracer",
    "MapperVolume",
    "MapperWireframer",
    "PartitionedDataSet",
    "Scene",
    "Tangle",
    "View3D",
    "cell_average",
    "contour",
    "contour_tree_augmented",
    "contour_tree_distributed",
    "create_tangle_dataset",
    "create_uniform_dataset",
    "distributed_branch_decomposition",
    "extract_top_volume_contours",
    "field_range",
    "gradient",
    "partition_uniform_dataset",
    "point_average",
    "render_dataset",
    "select_top_volume_branches",
    "vector_magnitude",
]
