##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from .. import _viskores

__all__ = []

if hasattr(_viskores, "ColorTable"):
    ColorTable = _viskores.ColorTable
    __all__.append("ColorTable")

for _name in sorted(dir(_viskores)):
    if _name.startswith(
        (
            "ArrayHandleGroupVecVariable",
            "ArrayHandleRecombineVec",
            "ArrayHandleSOAVec",
        )
    ):
        globals()[_name] = getattr(_viskores, _name)
        __all__.append(_name)

for _name in (
    "DeviceAdapterId",
    "Initialize",
    "InitializeOptions",
    "InitializeResult",
    "IsInitialized",
    "ArrayCopy",
    "CoordinateSystem",
    "DataSet",
    "DataSetBuilderCurvilinear",
    "DataSetBuilderExplicit",
    "DataSetBuilderExplicitIterative",
    "DataSetBuilderRectilinear",
    "DataSetBuilderUniform",
    "Field",
    "PartitionedDataSet",
    "UncertainArrayHandle",
    "UncertainCellSet",
    "UnknownArrayHandle",
    "UnknownCellSet",
    "make_Field",
    "make_DeviceAdapterId",
    "make_FieldCell",
    "make_FieldPoint",
):
    if hasattr(_viskores, _name):
        globals()[_name] = getattr(_viskores, _name)
        if _name not in __all__:
            __all__.append(_name)
