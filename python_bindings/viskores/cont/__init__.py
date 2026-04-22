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
    "ArrayHandleRecombineVecFloatDefault",
    "ArrayHandleSOAVec3f",
    "CellSet",
    "CoordinateSystem",
    "DataSet",
    "DataSetBuilderExplicit",
    "DataSetBuilderExplicitIterative",
    "Field",
    "PartitionedDataSet",
    "UnknownArrayHandle",
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
