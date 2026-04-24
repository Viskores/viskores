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

from viskores.filter.connected_components import CellSetConnectivity
from viskores.filter.contour import Contour
from viskores.source import Tangle
from viskores.testing import MakeTestDataSet


def count_components(dataset):
    values = dataset.GetField("component")
    return np.unique(values).size


def main():
    tangle = Tangle()
    tangle.SetCellDimensions((4, 4, 4))
    data_set = tangle.Execute()

    contour = Contour()
    contour.SetGenerateNormals(True)
    contour.SetMergeDuplicatePoints(True)
    contour.SetIsoValue(0, 0.1)
    contour.SetActiveField("tangle")
    iso = contour.Execute(data_set)

    connectivity = CellSetConnectivity()
    assert count_components(connectivity.Execute(iso)) == 8

    maker = MakeTestDataSet()
    assert count_components(connectivity.Execute(maker.Make3DExplicitDataSet5())) == 1
    assert count_components(connectivity.Execute(maker.Make3DUniformDataSet1())) == 1


if __name__ == "__main__":
    main()
