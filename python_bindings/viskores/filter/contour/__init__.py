##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from ... import _viskores

__all__ = []

if hasattr(_viskores, "contour"):
    contour = _viskores.contour
    __all__.append("contour")

if hasattr(_viskores, "Contour"):
    Contour = _viskores.Contour
    __all__.append("Contour")

if hasattr(_viskores, "ContourMarchingCells"):
    ContourMarchingCells = _viskores.ContourMarchingCells
    __all__.append("ContourMarchingCells")

if hasattr(_viskores, "ClipWithImplicitFunction"):
    ClipWithImplicitFunction = _viskores.ClipWithImplicitFunction
    __all__.append("ClipWithImplicitFunction")

if hasattr(_viskores, "ClipWithField"):
    ClipWithField = _viskores.ClipWithField
    __all__.append("ClipWithField")

if hasattr(_viskores, "MIRFilter"):
    MIRFilter = _viskores.MIRFilter
    __all__.append("MIRFilter")

if hasattr(_viskores, "Slice"):
    Slice = _viskores.Slice
    __all__.append("Slice")

if hasattr(_viskores, "SliceMultiple"):
    SliceMultiple = _viskores.SliceMultiple
    __all__.append("SliceMultiple")
