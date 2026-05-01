##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np

from viskores.filter import zfp
from viskores.testing import MakeTestDataSet


def assert_rate_property(filter_type):
    instance = filter_type()
    instance.SetRate(4.0)
    assert instance.GetRate() == 4.0


def assert_round_trip(dataset, compressor_type, decompressor_type):
    original = np.asarray(dataset.GetField("pointvar").GetData().AsNumPy()).copy()

    compressor = compressor_type()
    compressor.SetActiveField("pointvar")
    compressor.SetRate(4.0)
    compressed = compressor.Execute(dataset)

    decompressor = decompressor_type()
    decompressor.SetActiveField("compressed")
    decompressor.SetRate(4.0)
    decompressed = decompressor.Execute(compressed)

    np.testing.assert_allclose(decompressed.GetField("decompressed").GetData().AsNumPy(), original, atol=0.8)


def test_zfp_rate_properties():
    for filter_type in (
        zfp.ZFPCompressor1D,
        zfp.ZFPCompressor2D,
        zfp.ZFPCompressor3D,
        zfp.ZFPDecompressor1D,
        zfp.ZFPDecompressor2D,
        zfp.ZFPDecompressor3D,
    ):
        assert_rate_property(filter_type)


def test_zfp_1d_round_trip():
    dataset = MakeTestDataSet().Make1DUniformDataSet2()
    assert_round_trip(dataset, zfp.ZFPCompressor1D, zfp.ZFPDecompressor1D)


def test_zfp_2d_round_trip():
    dataset = MakeTestDataSet().Make2DUniformDataSet2()
    assert_round_trip(dataset, zfp.ZFPCompressor2D, zfp.ZFPDecompressor2D)


def main():
    test_zfp_rate_properties()
    test_zfp_1d_round_trip()
    test_zfp_2d_round_trip()


if __name__ == "__main__":
    main()
