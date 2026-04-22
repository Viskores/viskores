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
from viskores.filter.contour import Contour
from viskores.filter.field_conversion import CellAverage
from viskores.filter.vector_analysis import Gradient, VectorMagnitude


def main():
    dims = (4, 3, 2)
    ds = viskores.create_uniform_dataset(dims, origin=(0.0, 0.0, 0.0), spacing=(1.0, 2.0, 3.0))

    point_count = np.prod(dims)
    scalar = np.linspace(0.0, 1.0, point_count, dtype=np.float64)
    vector = np.stack([scalar, scalar * 2.0, scalar * 3.0], axis=1)

    ds.AddPointField("temperature", scalar)
    ds.AddPointField("velocity", vector)

    avg_filter = CellAverage()
    avg_filter.SetActiveField("temperature")
    avg_filter.SetOutputFieldName("temperature_cells")
    avg = avg_filter.Execute(ds)

    grad_filter = Gradient()
    grad_filter.SetActiveField("temperature")
    grad_filter.SetOutputFieldName("temperature_grad")
    grad = grad_filter.Execute(ds)

    mag_filter = VectorMagnitude()
    mag_filter.SetActiveField("velocity")
    mag_filter.SetOutputFieldName("speed")
    mag = mag_filter.Execute(ds)

    contour_filter = Contour()
    contour_filter.SetActiveField("temperature")
    contour_filter.SetIsoValue(0.5)
    iso = contour_filter.Execute(ds)

    assert ds.GetNumberOfPoints() == point_count
    assert avg.GetField("temperature_cells").ndim == 1
    assert grad.GetField("temperature_grad").ndim == 2
    assert grad.GetField("temperature_grad").shape[1] >= 3
    assert mag.GetField("speed").ndim == 1
    assert iso.GetNumberOfCells() > 0


if __name__ == "__main__":
    main()
