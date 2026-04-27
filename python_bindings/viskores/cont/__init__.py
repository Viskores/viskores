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
    "DataSetBuilderExplicit",
    "DataSetBuilderExplicitIterative",
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
