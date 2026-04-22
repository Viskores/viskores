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

if hasattr(_viskores, "CellSetConnectivity"):
    CellSetConnectivity = _viskores.CellSetConnectivity
    __all__.append("CellSetConnectivity")

if hasattr(_viskores, "ImageConnectivity"):
    ImageConnectivity = _viskores.ImageConnectivity
    __all__.append("ImageConnectivity")
