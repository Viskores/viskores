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

if hasattr(_viskores, "MergeDataSets"):
    MergeDataSets = _viskores.MergeDataSets
    __all__.append("MergeDataSets")
