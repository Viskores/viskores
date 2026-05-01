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

if hasattr(_viskores, "MakeTestDataSet"):
    MakeTestDataSet = _viskores.MakeTestDataSet
    __all__.append("MakeTestDataSet")

for _name in (
    "can_narrow_to_coordinate_system_data",
    "can_narrow_to_structured_cell_set",
):
    if hasattr(_viskores, _name):
        globals()[_name] = getattr(_viskores, _name)
        __all__.append(_name)
