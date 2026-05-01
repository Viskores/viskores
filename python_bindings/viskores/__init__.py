##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from . import _viskores
from . import cont, filter, interop, io, python_convenience, rendering, source, testing

__all__ = [
    "cont",
    "filter",
    "interop",
    "io",
    "python_convenience",
    "rendering",
    "source",
    "testing",
]

for _name in (
    "Box",
    "Bounds",
    "CELL_SHAPE_HEXAHEDRON",
    "CELL_SHAPE_LINE",
    "CELL_SHAPE_POLYGON",
    "CELL_SHAPE_POLY_LINE",
    "CELL_SHAPE_PYRAMID",
    "CELL_SHAPE_QUAD",
    "CELL_SHAPE_TETRA",
    "CELL_SHAPE_TRIANGLE",
    "CELL_SHAPE_VERTEX",
    "CELL_SHAPE_WEDGE",
    "ColorSpace",
    "Cylinder",
    "ImplicitFunctionGeneral",
    "Plane",
    "Range",
    "Sphere",
):
    if hasattr(_viskores, _name) and _name not in __all__:
        globals()[_name] = getattr(_viskores, _name)
        __all__.append(_name)
