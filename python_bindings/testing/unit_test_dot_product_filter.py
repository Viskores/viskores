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

from viskores.filter.vector_analysis import DotProduct
from viskores.testing import MakeTestDataSet


def create_vectors(num_pts, vec_type):
    if vec_type == 0:
        vecs1 = np.tile(np.array([[1.0, 0.0, 0.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[0.0, 1.0, 0.0]]), (num_pts, 1))
    elif vec_type == 1:
        vecs1 = np.tile(np.array([[0.0, 1.0, 0.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[0.0, 0.0, 1.0]]), (num_pts, 1))
    elif vec_type == 2:
        vecs1 = np.tile(np.array([[0.0, 0.0, 1.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[1.0, 0.0, 0.0]]), (num_pts, 1))
    elif vec_type == 3:
        vecs1 = np.tile(np.array([[0.0, 1.0, 0.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[1.0, 0.0, 0.0]]), (num_pts, 1))
    elif vec_type == 4:
        vecs1 = np.tile(np.array([[0.0, 0.0, 1.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[0.0, 1.0, 0.0]]), (num_pts, 1))
    elif vec_type == 5:
        vecs1 = np.tile(np.array([[1.0, 0.0, 0.0]]), (num_pts, 1))
        vecs2 = np.tile(np.array([[0.0, 0.0, 1.0]]), (num_pts, 1))
    else:
        rng = np.random.default_rng(12345)
        vecs1 = rng.uniform(-10.0, 10.0, size=(num_pts, 3))
        vecs2 = rng.uniform(-10.0, 10.0, size=(num_pts, 3))
    return vecs1.astype(np.float32), vecs2.astype(np.float32)


def check_result(field1, field2, result):
    output = result.GetField("dotproduct")
    expected = np.sum(field1 * field2, axis=1)
    assert output.shape == expected.shape
    assert np.allclose(output, expected, rtol=1e-5, atol=1e-5)


def main():
    maker = MakeTestDataSet()
    for case in range(7):
        dataset = maker.Make3DUniformDataSet0()
        num_points = dataset.GetNumberOfPoints()
        field1, field2 = create_vectors(num_points, case)
        dataset.AddPointField("vec1", field1)
        dataset.AddPointField("vec2", field2)
        dataset.AddCoordinateSystem("vecA", field1)
        dataset.AddCoordinateSystem("vecB", field2)

        filter_obj = DotProduct()
        filter_obj.SetPrimaryField("vec1")
        filter_obj.SetSecondaryField("vec2")
        result = filter_obj.Execute(dataset)
        check_result(field1, field2, result)

        filter_obj = DotProduct()
        filter_obj.SetUseCoordinateSystemAsPrimaryField(True)
        filter_obj.SetPrimaryCoordinateSystem(1)
        filter_obj.SetSecondaryField("vec2")
        result = filter_obj.Execute(dataset)
        check_result(field1, field2, result)

        filter_obj = DotProduct()
        filter_obj.SetPrimaryField("vec1")
        filter_obj.SetUseCoordinateSystemAsSecondaryField(True)
        filter_obj.SetSecondaryCoordinateSystem(2)
        result = filter_obj.Execute(dataset)
        check_result(field1, field2, result)


if __name__ == "__main__":
    main()
