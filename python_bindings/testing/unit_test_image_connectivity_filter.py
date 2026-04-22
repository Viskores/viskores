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

from viskores.cont import create_uniform_dataset
from viskores.filter.connected_components import ImageConnectivity


def main():
    pixels = np.array(
        [
            0, 1, 1, 1, 0, 1, 1, 0,
            0, 0, 0, 1, 0, 1, 1, 0,
            0, 1, 1, 0, 0, 1, 1, 0,
            0, 1, 0, 0, 0, 1, 1, 0,
            0, 1, 0, 1, 1, 1, 1, 1,
            0, 1, 0, 1, 1, 1, 1, 1,
            0, 1, 0, 1, 1, 1, 0, 0,
            0, 1, 1, 1, 1, 1, 0, 0,
        ],
        dtype=np.uint8,
    )
    expected = np.array(
        [
            0, 1, 1, 1, 0, 1, 1, 2,
            0, 0, 0, 1, 0, 1, 1, 2,
            0, 1, 1, 0, 0, 1, 1, 2,
            0, 1, 0, 0, 0, 1, 1, 2,
            0, 1, 0, 1, 1, 1, 1, 1,
            0, 1, 0, 1, 1, 1, 1, 1,
            0, 1, 0, 1, 1, 1, 3, 3,
            0, 1, 1, 1, 1, 1, 3, 3,
        ],
        dtype=np.int64,
    )

    dataset = create_uniform_dataset((8, 8, 1))
    dataset.AddPointField("color", pixels)

    connectivity = ImageConnectivity()
    connectivity.SetActiveField("color")
    output = connectivity.Execute(dataset)
    result = output.GetField("component")

    assert np.array_equal(result, expected)


if __name__ == "__main__":
    main()
