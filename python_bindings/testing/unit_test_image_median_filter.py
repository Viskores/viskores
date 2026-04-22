##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import math

import viskores.cont
from viskores.filter.image_processing import ImageMedian
from viskores.testing import MakeTestDataSet


def main():
    viskores.cont.Initialize(["unit_test_image_median_filter.py"])

    dataset = MakeTestDataSet().Make3DUniformDataSet2()

    median = ImageMedian()
    median.Perform3x3()
    median.SetActiveField("pointvar")
    result = median.Execute(dataset)

    assert result.HasField("median", association="points")
    values = result.GetField("median")
    point_dims = (64, 64, 64)

    assert math.isclose(float(values[1 + point_dims[0]]), 2.0, rel_tol=1e-6, abs_tol=1e-6)
    index = 1 + point_dims[0] + (point_dims[1] * point_dims[0] * 2)
    assert math.isclose(float(values[index]), 2.82843, rel_tol=1e-5, abs_tol=1e-5)


if __name__ == "__main__":
    main()
