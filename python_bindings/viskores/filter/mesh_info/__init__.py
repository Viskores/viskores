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

from ... import _viskores

__all__ = []

if all(
    hasattr(_viskores, name)
    for name in (
        "INTEGRATION_TYPE_NONE",
        "INTEGRATION_TYPE_ARC_LENGTH",
        "INTEGRATION_TYPE_AREA",
        "INTEGRATION_TYPE_VOLUME",
        "INTEGRATION_TYPE_ALL_MEASURES",
    )
):

    class IntegrationType(IntEnum):
        None_ = _viskores.INTEGRATION_TYPE_NONE
        ArcLength = _viskores.INTEGRATION_TYPE_ARC_LENGTH
        Area = _viskores.INTEGRATION_TYPE_AREA
        Volume = _viskores.INTEGRATION_TYPE_VOLUME
        AllMeasures = _viskores.INTEGRATION_TYPE_ALL_MEASURES

    __all__.append("IntegrationType")

if all(
    hasattr(_viskores, name)
    for name in (
        "CELL_METRIC_AREA",
        "CELL_METRIC_ASPECT_GAMMA",
        "CELL_METRIC_ASPECT_RATIO",
        "CELL_METRIC_CONDITION",
        "CELL_METRIC_DIAGONAL_RATIO",
        "CELL_METRIC_DIMENSION",
        "CELL_METRIC_JACOBIAN",
        "CELL_METRIC_MAX_ANGLE",
        "CELL_METRIC_MAX_DIAGONAL",
        "CELL_METRIC_MIN_ANGLE",
        "CELL_METRIC_MIN_DIAGONAL",
        "CELL_METRIC_ODDY",
        "CELL_METRIC_RELATIVE_SIZE_SQUARED",
        "CELL_METRIC_SCALED_JACOBIAN",
        "CELL_METRIC_SHAPE",
        "CELL_METRIC_SHAPE_AND_SIZE",
        "CELL_METRIC_SHEAR",
        "CELL_METRIC_SKEW",
        "CELL_METRIC_STRETCH",
        "CELL_METRIC_TAPER",
        "CELL_METRIC_VOLUME",
        "CELL_METRIC_WARPAGE",
        "CELL_METRIC_NONE",
    )
):

    class CellMetric(IntEnum):
        Area = _viskores.CELL_METRIC_AREA
        AspectGamma = _viskores.CELL_METRIC_ASPECT_GAMMA
        AspectRatio = _viskores.CELL_METRIC_ASPECT_RATIO
        Condition = _viskores.CELL_METRIC_CONDITION
        DiagonalRatio = _viskores.CELL_METRIC_DIAGONAL_RATIO
        Dimension = _viskores.CELL_METRIC_DIMENSION
        Jacobian = _viskores.CELL_METRIC_JACOBIAN
        MaxAngle = _viskores.CELL_METRIC_MAX_ANGLE
        MaxDiagonal = _viskores.CELL_METRIC_MAX_DIAGONAL
        MinAngle = _viskores.CELL_METRIC_MIN_ANGLE
        MinDiagonal = _viskores.CELL_METRIC_MIN_DIAGONAL
        Oddy = _viskores.CELL_METRIC_ODDY
        RelativeSizeSquared = _viskores.CELL_METRIC_RELATIVE_SIZE_SQUARED
        ScaledJacobian = _viskores.CELL_METRIC_SCALED_JACOBIAN
        Shape = _viskores.CELL_METRIC_SHAPE
        ShapeAndSize = _viskores.CELL_METRIC_SHAPE_AND_SIZE
        Shear = _viskores.CELL_METRIC_SHEAR
        Skew = _viskores.CELL_METRIC_SKEW
        Stretch = _viskores.CELL_METRIC_STRETCH
        Taper = _viskores.CELL_METRIC_TAPER
        Volume = _viskores.CELL_METRIC_VOLUME
        Warpage = _viskores.CELL_METRIC_WARPAGE
        None_ = _viskores.CELL_METRIC_NONE

    __all__.append("CellMetric")

if hasattr(_viskores, "CellMeasures"):
    CellMeasures = _viskores.CellMeasures
    __all__.append("CellMeasures")

if hasattr(_viskores, "GhostCellClassify"):
    GhostCellClassify = _viskores.GhostCellClassify
    __all__.append("GhostCellClassify")

if hasattr(_viskores, "MeshQuality"):
    MeshQuality = _viskores.MeshQuality
    __all__.append("MeshQuality")
