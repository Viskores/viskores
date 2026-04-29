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

import numpy as np

from .. import _viskores

__all__ = []

if all(
    hasattr(_viskores, name)
    for name in (
        "ASSOCIATION_ANY",
        "ASSOCIATION_CELLS",
        "ASSOCIATION_GLOBAL",
        "ASSOCIATION_PARTITIONS",
        "ASSOCIATION_POINTS",
        "ASSOCIATION_WHOLE_DATASET",
    )
):

    class Association(IntEnum):
        ANY = _viskores.ASSOCIATION_ANY
        WHOLE_DATASET = _viskores.ASSOCIATION_WHOLE_DATASET
        POINTS = _viskores.ASSOCIATION_POINTS
        CELLS = _viskores.ASSOCIATION_CELLS
        PARTITIONS = _viskores.ASSOCIATION_PARTITIONS
        GLOBAL = _viskores.ASSOCIATION_GLOBAL

    __all__.append("Association")

if hasattr(_viskores, "ColorTable"):
    ColorTable = _viskores.ColorTable
    __all__.append("ColorTable")

for _name in (
    "DeviceAdapterId",
    "Initialize",
    "InitializeOptions",
    "InitializeResult",
    "IsInitialized",
    "ArrayCopy",
    "ArrayHandleRecombineVecFloat32",
    "ArrayHandleRecombineVecFloat64",
    "ArrayHandleRecombineVecFloatDefault",
    "ArrayHandleRecombineVecInt8",
    "ArrayHandleRecombineVecUInt8",
    "ArrayHandleRecombineVecInt16",
    "ArrayHandleRecombineVecUInt16",
    "ArrayHandleRecombineVecInt32",
    "ArrayHandleRecombineVecUInt32",
    "ArrayHandleRecombineVecInt64",
    "ArrayHandleRecombineVecUInt64",
    "ArrayHandleGroupVecVariableFloat32",
    "ArrayHandleGroupVecVariableFloat64",
    "ArrayHandleGroupVecVariableFloatDefault",
    "ArrayHandleGroupVecVariableInt8",
    "ArrayHandleGroupVecVariableUInt8",
    "ArrayHandleGroupVecVariableInt16",
    "ArrayHandleGroupVecVariableUInt16",
    "ArrayHandleGroupVecVariableInt32",
    "ArrayHandleGroupVecVariableUInt32",
    "ArrayHandleGroupVecVariableInt64",
    "ArrayHandleGroupVecVariableUInt64",
    "ArrayHandleGroupVecVariableId",
    "ArrayHandleSOAVec2f_32",
    "ArrayHandleSOAVec2f_64",
    "ArrayHandleSOAVec3f_32",
    "ArrayHandleSOAVec3f_64",
    "ArrayHandleSOAVec4f_32",
    "ArrayHandleSOAVec4f_64",
    "ArrayHandleSOAVec3f",
    "ArrayHandleSOAVec2i_8",
    "ArrayHandleSOAVec2i_16",
    "ArrayHandleSOAVec2i_32",
    "ArrayHandleSOAVec2i_64",
    "ArrayHandleSOAVec3i_8",
    "ArrayHandleSOAVec3i_16",
    "ArrayHandleSOAVec3i_32",
    "ArrayHandleSOAVec3i_64",
    "ArrayHandleSOAVec4i_8",
    "ArrayHandleSOAVec4i_16",
    "ArrayHandleSOAVec4i_32",
    "ArrayHandleSOAVec4i_64",
    "ArrayHandleSOAVec2ui_8",
    "ArrayHandleSOAVec2ui_16",
    "ArrayHandleSOAVec2ui_32",
    "ArrayHandleSOAVec2ui_64",
    "ArrayHandleSOAVec3ui_8",
    "ArrayHandleSOAVec3ui_16",
    "ArrayHandleSOAVec3ui_32",
    "ArrayHandleSOAVec3ui_64",
    "ArrayHandleSOAVec4ui_8",
    "ArrayHandleSOAVec4ui_16",
    "ArrayHandleSOAVec4ui_32",
    "ArrayHandleSOAVec4ui_64",
    "CellSet",
    "CoordinateSystem",
    "DataSet",
    "DataSetBuilderCurvilinear",
    "DataSetBuilderExplicit",
    "DataSetBuilderExplicitIterative",
    "DataSetBuilderRectilinear",
    "DataSetBuilderUniform",
    "Field",
    "PartitionedDataSet",
    "UnknownArrayHandle",
    "array_from_numpy",
    "asnumpy",
    "create_uniform_dataset",
    "make_Field",
    "make_DeviceAdapterId",
    "make_FieldCell",
    "make_FieldPoint",
    "make_FieldWholeDataSet",
    "partition_uniform_dataset",
):
    if hasattr(_viskores, _name):
        globals()[_name] = getattr(_viskores, _name)
        __all__.append(_name)


_create_uniform_dataset = create_uniform_dataset


def _normalize_dimensions(dimensions):
    if dimensions is None:
        return None
    if isinstance(dimensions, int):
        dimensions = (dimensions,)
    normalized = tuple(int(dimension) for dimension in dimensions)
    if not (1 <= len(normalized) <= 3):
        raise ValueError("Uniform dataset dimensions must have 1, 2, or 3 entries.")
    if any(dimension <= 0 for dimension in normalized):
        raise ValueError("Uniform dataset dimensions must all be positive.")
    return normalized


def _require_field_mapping(fields, association):
    if fields is None:
        return ()
    if not hasattr(fields, "items"):
        raise TypeError(f"{association}_fields must be a mapping of field name to NumPy array.")
    return fields.items()


def _infer_uniform_dimensions(point_fields, cell_fields):
    for association, fields, offset in (
        ("point", point_fields, 0),
        ("cell", cell_fields, 1),
    ):
        for field_name, values in _require_field_mapping(fields, association):
            array = np.asarray(values)
            if array.ndim > 1:
                dimensions = tuple(int(size) + offset for size in array.shape)
                if 1 <= len(dimensions) <= 3:
                    return dimensions
                raise ValueError(
                    f"Cannot infer uniform dataset dimensions from {association} field "
                    f"{field_name!r} with shape {array.shape}."
                )
    raise ValueError(
        "dimensions must be provided unless a shaped point_fields or cell_fields array "
        "can be used to infer them."
    )


def _format_expected_shape(shape):
    return "(" + ", ".join(str(size) for size in shape) + ")"


def _prepare_uniform_field_array(field_name, values, grid_shape, association):
    array = np.asarray(values)
    if not array.flags.c_contiguous:
        raise ValueError(
            f"{association} field {field_name!r} must be C-contiguous to be added without copy."
        )

    number_of_values = int(np.prod(grid_shape, dtype=np.int64))
    field_shape = tuple(int(size) for size in array.shape)
    flat_scalar_shape = (number_of_values,)
    expected_grid_shape = tuple(int(size) for size in grid_shape)

    if field_shape == expected_grid_shape:
        return array.reshape(flat_scalar_shape)

    if field_shape == flat_scalar_shape:
        return array

    if (
        array.ndim == len(expected_grid_shape) + 1
        and field_shape[:-1] == expected_grid_shape
        and field_shape[-1] in (1, 2, 3, 4)
    ):
        return array.reshape(number_of_values, field_shape[-1])

    if (
        array.ndim == 2
        and field_shape[0] == number_of_values
        and field_shape[1] in (1, 2, 3, 4)
    ):
        return array

    component_shape = expected_grid_shape + ("components",)
    raise ValueError(
        f"{association} field {field_name!r} has shape {array.shape}; expected "
        f"{_format_expected_shape(expected_grid_shape)}, "
        f"{_format_expected_shape(component_shape)}, ({number_of_values},), or "
        f"({number_of_values}, components)."
    )


def _add_uniform_fields(dataset, fields, grid_shape, association):
    for field_name, values in _require_field_mapping(fields, association):
        if not isinstance(field_name, str):
            raise TypeError(f"{association} field names must be strings.")
        field_values = _prepare_uniform_field_array(field_name, values, grid_shape, association)
        field_array = array_from_numpy(field_values, copy=False)
        if association == "point":
            dataset.AddPointField(field_name, field_array)
        else:
            dataset.AddCellField(field_name, field_array)


def create_uniform_dataset(
    dimensions=None,
    origin=None,
    spacing=None,
    coord_name="coords",
    *,
    point_fields=None,
    cell_fields=None,
):
    """Create a uniform structured DataSet from dimensions, origin, and spacing.

    ``point_fields`` and ``cell_fields`` may map field names to NumPy arrays.
    Point field arrays may have shape ``dimensions`` or a flat shape with one
    value per point. Cell field arrays may have shape ``dimensions - 1`` or a
    flat shape with one value per cell. Vector-valued fields may use a trailing
    component axis of length 1, 2, 3, or 4 when ``dimensions`` is provided. If
    ``dimensions`` is omitted, it is inferred from the first shaped field as a
    scalar grid. C-contiguous arrays are reshaped and added through
    ``array_from_numpy(..., copy=False)`` so the field data can be shared with
    Viskores without copying.
    """

    dimensions = _normalize_dimensions(dimensions)
    if dimensions is None:
        dimensions = _infer_uniform_dimensions(point_fields, cell_fields)

    dataset = _create_uniform_dataset(
        dimensions,
        origin=origin,
        spacing=spacing,
        coord_name=coord_name,
    )

    point_shape = dimensions
    cell_shape = tuple(max(dimension - 1, 0) for dimension in dimensions)
    _add_uniform_fields(dataset, point_fields, point_shape, "point")
    _add_uniform_fields(dataset, cell_fields, cell_shape, "cell")
    return dataset
