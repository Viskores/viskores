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

from viskores.filter.mesh_info import CellMeasures, IntegrationType
from viskores.testing import MakeTestDataSet


def check_cell_measures(dataset, expected, integration_type):
    cell_measures = CellMeasures()
    cell_measures.SetMeasure(integration_type)
    output = cell_measures.Execute(dataset)
    assert cell_measures.GetCellMeasureName() == "measure"
    assert output.GetNumberOfCells() == len(expected)
    result = output.GetField(cell_measures.GetCellMeasureName())
    assert result.shape[0] == len(expected)
    assert np.allclose(result, np.array(expected, dtype=result.dtype), atol=1.0e-5)


def main():
    maker = MakeTestDataSet()

    check_cell_measures(
        maker.Make3DExplicitDataSet2(),
        [-1.0],
        IntegrationType.AllMeasures,
    )
    check_cell_measures(
        maker.Make3DExplicitDataSet3(),
        [-1.0 / 6.0],
        IntegrationType.AllMeasures,
    )
    check_cell_measures(
        maker.Make3DExplicitDataSet4(),
        [-1.0, -1.0],
        IntegrationType.AllMeasures,
    )
    check_cell_measures(
        maker.Make3DExplicitDataSet5(),
        [1.0, 1.0 / 3.0, 1.0 / 6.0, -1.0 / 2.0],
        IntegrationType.AllMeasures,
    )
    check_cell_measures(
        maker.Make3DExplicitDataSet6(),
        [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.083426, 0.25028],
        IntegrationType.Volume,
    )
    check_cell_measures(
        maker.Make3DExplicitDataSet6(),
        [0.999924, 0.999924, 0.0, 0.0, 3.85516, 1.00119, 0.083426, 0.25028],
        IntegrationType.AllMeasures,
    )


if __name__ == "__main__":
    main()
