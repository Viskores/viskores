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

if all(hasattr(_viskores, name) for name in ("FILE_TYPE_ASCII", "FILE_TYPE_BINARY")):

    class FileType(IntEnum):
        ASCII = _viskores.FILE_TYPE_ASCII
        BINARY = _viskores.FILE_TYPE_BINARY

    __all__.append("FileType")

if all(hasattr(_viskores, name) for name in ("PIXEL_DEPTH_8", "PIXEL_DEPTH_16")):

    class PixelDepth(IntEnum):
        PIXEL_8 = _viskores.PIXEL_DEPTH_8
        PIXEL_16 = _viskores.PIXEL_DEPTH_16

    __all__.append("PixelDepth")

for _name in (
    "BOVDataSetReader",
    "ImageReaderHDF5",
    "ImageReaderPNG",
    "ImageReaderPNM",
    "ImageWriterHDF5",
    "ImageWriterPNG",
    "ImageWriterPNM",
    "VTKDataSetReader",
    "VTKDataSetWriter",
    "VTKVisItFileReader",
):
    if hasattr(_viskores, _name):
        globals()[_name] = getattr(_viskores, _name)
        __all__.append(_name)
