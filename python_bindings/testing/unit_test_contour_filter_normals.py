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

import viskores
from viskores.filter.clean_grid import CleanGrid
from viskores.filter.contour import Contour


def make_normals_test_dataset():
    dataset = viskores.create_uniform_dataset((3, 4, 4))
    values = np.array(
        [
            60.764,
            107.555,
            80.524,
            63.639,
            131.087,
            83.4,
            98.161,
            165.608,
            117.921,
            37.353,
            84.145,
            57.114,
            95.202,
            162.649,
            114.962,
            115.896,
            215.56,
            135.657,
            150.418,
            250.081,
            170.178,
            71.791,
            139.239,
            91.552,
            95.202,
            162.649,
            114.962,
            115.896,
            215.56,
            135.657,
            150.418,
            250.081,
            170.178,
            71.791,
            139.239,
            91.552,
            60.764,
            107.555,
            80.524,
            63.639,
            131.087,
            83.4,
            98.161,
            165.608,
            117.921,
            37.353,
            84.145,
            57.114,
        ],
        dtype=np.float32,
    )
    dataset.AddPointField("pointvar", values)
    return dataset


def assert_normals_match(computed, expected, ordering=None):
    if ordering is None:
        np.testing.assert_allclose(computed, expected, rtol=1e-3, atol=1e-3)
        return
    np.testing.assert_allclose(computed, expected[ordering], rtol=1e-3, atol=1e-3)


def test_structured_normals():
    dataset = make_normals_test_dataset()
    contour = Contour()
    contour.SetIsoValue(200.0)
    contour.SetGenerateNormals(True)
    contour.SetComputeFastNormals(False)
    contour.SetActiveField("pointvar")

    result = contour.Execute(dataset)
    normals = result.GetField("normals")
    expected = np.array(
        [
            [0.151008, 0.626778, 0.764425],
            [0.133328, -0.397444, 0.907889],
            [0.162649, 0.764163, 0.62418],
            [0.385327, 0.664323, 0.640467],
            [-0.13372, 0.713645, 0.687626],
            [0.770536, -0.421248, 0.478356],
            [-0.736036, -0.445244, 0.50991],
            [0.123446, -0.887088, 0.444788],
            [0.162649, 0.764163, -0.62418],
            [0.385327, 0.664323, -0.640467],
            [-0.13372, 0.713645, -0.687626],
            [0.151008, 0.626778, -0.764425],
            [0.770536, -0.421248, -0.478356],
            [-0.736036, -0.445244, -0.50991],
            [0.123446, -0.887088, -0.444788],
            [0.133328, -0.397444, -0.907889],
        ],
        dtype=np.float32,
    )
    ordering = np.array([0, 1, 3, 5, 4, 6, 2, 7, 9, 12, 10, 13, 8, 14, 11, 15], dtype=np.int64)
    if np.allclose(normals, expected[ordering], rtol=1e-3, atol=1e-3):
        assert_normals_match(normals, expected, ordering)
    else:
        assert_normals_match(normals, expected)

    contour.SetComputeFastNormals(True)
    result = contour.Execute(dataset)
    fast_normals = result.GetField("normals")
    expected_fast = np.array(
        [
            [-0.1351, 0.4377, 0.8889],
            [0.2863, -0.1721, 0.9426],
            [0.3629, 0.8155, 0.4509],
            [0.8486, 0.356, 0.3914],
            [-0.8315, 0.4727, 0.2917],
            [0.9395, -0.253, 0.2311],
            [-0.9105, -0.0298, 0.4124],
            [-0.1078, -0.9585, 0.2637],
            [-0.2538, 0.8534, -0.4553],
            [0.8953, 0.3902, -0.2149],
            [-0.8295, 0.4188, -0.3694],
            [0.2434, 0.4297, -0.8695],
            [0.8951, -0.1347, -0.4251],
            [-0.8467, -0.4258, -0.3191],
            [0.2164, -0.9401, -0.2635],
            [-0.1589, -0.1642, -0.9735],
        ],
        dtype=np.float32,
    )
    expected_fast_fe_y = np.array(
        [
            [0.243433, 0.429741, 0.869519],
            [-0.158904, -0.164214, 0.973542],
            [0.895292, 0.390217, 0.214903],
            [0.895057, -0.134692, 0.425125],
            [-0.829547, 0.418793, 0.36941],
            [-0.846705, -0.425787, 0.319054],
            [-0.253811, 0.853394, 0.4553],
            [0.216381, -0.940084, 0.263478],
            [0.848579, 0.35602, -0.391362],
            [0.93948, -0.252957, -0.231065],
            [-0.831549, 0.472663, -0.291744],
            [-0.910494, -0.0298277, -0.412446],
            [0.362862, 0.815464, -0.450944],
            [-0.107848, -0.958544, -0.263748],
            [-0.135131, 0.437674, -0.888921],
            [0.286251, -0.172078, -0.942576],
        ],
        dtype=np.float32,
    )
    if np.allclose(fast_normals, expected_fast_fe_y, rtol=1e-3, atol=1e-3):
        assert_normals_match(fast_normals, expected_fast_fe_y)
    else:
        assert_normals_match(fast_normals, expected_fast)


def test_unstructured_normals():
    dataset = make_normals_test_dataset()
    clean = CleanGrid()
    clean.SetCompactPointFields(False)
    clean.SetMergePoints(False)
    clean.SetFieldsToPass("pointvar")
    unstructured = clean.Execute(dataset)

    contour = Contour()
    contour.SetIsoValue(200.0)
    contour.SetGenerateNormals(True)
    contour.SetComputeFastNormals(True)
    contour.SetActiveField("pointvar")

    fast_result = contour.Execute(unstructured)
    fast_normals = fast_result.GetField("normals")
    expected_fast = np.array(
        [
            [-0.1351, 0.4377, 0.8889],
            [0.2863, -0.1721, 0.9426],
            [0.3629, 0.8155, 0.4509],
            [0.8486, 0.356, 0.3914],
            [-0.8315, 0.4727, 0.2917],
            [0.9395, -0.253, 0.2311],
            [-0.9105, -0.0298, 0.4124],
            [-0.1078, -0.9585, 0.2637],
            [-0.2538, 0.8534, -0.4553],
            [0.8953, 0.3902, -0.2149],
            [-0.8295, 0.4188, -0.3694],
            [0.2434, 0.4297, -0.8695],
            [0.8951, -0.1347, -0.4251],
            [-0.8467, -0.4258, -0.3191],
            [0.2164, -0.9401, -0.2635],
            [-0.1589, -0.1642, -0.9735],
        ],
        dtype=np.float32,
    )
    np.testing.assert_allclose(fast_normals, expected_fast, rtol=1e-3, atol=1e-3)

    contour.SetComputeFastNormals(False)
    result = contour.Execute(unstructured)
    normals = result.GetField("normals")
    expected = np.array(
        [
            [0.1510, 0.6268, 0.7644],
            [0.1333, -0.3974, 0.9079],
            [0.1626, 0.7642, 0.6242],
            [0.3853, 0.6643, 0.6405],
            [-0.1337, 0.7136, 0.6876],
            [0.7705, -0.4212, 0.4784],
            [-0.7360, -0.4452, 0.5099],
            [0.1234, -0.8871, 0.4448],
            [0.1626, 0.7642, -0.6242],
            [0.3853, 0.6643, -0.6405],
            [-0.1337, 0.7136, -0.6876],
            [0.1510, 0.6268, -0.7644],
            [0.7705, -0.4212, -0.4784],
            [-0.7360, -0.4452, -0.5099],
            [0.1234, -0.8871, -0.4448],
            [0.1333, -0.3974, -0.9079],
        ],
        dtype=np.float32,
    )
    np.testing.assert_allclose(normals, expected, rtol=1e-3, atol=1e-3)


def main():
    test_structured_normals()
    test_unstructured_normals()


if __name__ == "__main__":
    main()
