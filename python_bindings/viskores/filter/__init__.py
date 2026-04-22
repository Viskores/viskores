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

if hasattr(_viskores, "CleanGrid"):
    from . import clean_grid

    __all__.append("clean_grid")

if hasattr(_viskores, "CellSetConnectivity") or hasattr(_viskores, "ImageConnectivity"):
    from . import connected_components

    __all__.append("connected_components")

if hasattr(_viskores, "contour"):
    from . import contour

    __all__.append("contour")

if hasattr(_viskores, "Threshold") or hasattr(_viskores, "ThresholdPoints"):
    from . import entity_extraction

    __all__.append("entity_extraction")

if any(
    hasattr(_viskores, name)
    for name in (
        "cell_average",
        "point_average",
    )
):
    from . import field_conversion

    __all__.append("field_conversion")

if any(
    hasattr(_viskores, name)
    for name in (
        "CompositeVectors",
        "CylindricalCoordinateTransform",
        "FieldToColors",
        "GenerateIds",
        "LogValues",
        "PointElevation",
        "PointTransform",
        "SphericalCoordinateTransform",
        "Warp",
    )
):
    from . import field_transform

    __all__.append("field_transform")

if any(
    hasattr(_viskores, name)
    for name in (
        "ContinuousScatterPlot",
        "Entropy",
        "Histogram",
        "NDEntropy",
        "NDHistogram",
        "ParticleDensityCloudInCell",
        "ParticleDensityNearestGridPoint",
        "Statistics",
    )
):
    from . import density_estimate

    __all__.append("density_estimate")

if any(
    hasattr(_viskores, name)
    for name in (
        "ComputeMoments",
        "ImageDifference",
        "ImageMedian",
    )
):
    from . import image_processing

    __all__.append("image_processing")

if any(
    hasattr(_viskores, name)
    for name in (
        "ContourTreeAugmented",
        "ContourTreeUniformDistributed",
        "DistributedBranchDecompositionFilter",
        "ExtractTopVolumeContoursFilter",
        "SelectTopVolumeBranchesFilter",
    )
):
    from . import scalar_topology

    __all__.append("scalar_topology")

if any(
    hasattr(_viskores, name)
    for name in (
        "gradient",
        "vector_magnitude",
    )
):
    from . import vector_analysis

    __all__.append("vector_analysis")

if hasattr(_viskores, "CellMeasures"):
    from . import mesh_info

    __all__.append("mesh_info")

if hasattr(_viskores, "MergeDataSets"):
    from . import multi_block

    __all__.append("multi_block")

if any(
    hasattr(_viskores, name)
    for name in (
        "HistSampling",
        "Probe",
    )
):
    from . import resampling

    __all__.append("resampling")

if hasattr(_viskores, "Tetrahedralize"):
    from . import geometry_refinement

    __all__.append("geometry_refinement")
