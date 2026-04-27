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
from viskores.filter.mesh_info import GhostCellClassify


def required_num_normal_cells(nx, ny, nz, layer):
    cells = nx - 2 * layer
    if ny > 0:
        cells *= ny - 2 * layer
    if nz > 0:
        cells *= nz - 2 * layer
    return cells


def run_case(nx, ny, nz, ghost_name):
    dims = (nx + 1,) if ny == 0 and nz == 0 else (nx + 1, ny + 1) if nz == 0 else (nx + 1, ny + 1, nz + 1)
    dataset = viskores.create_uniform_dataset(dims)

    classify = GhostCellClassify()
    classify.SetGhostCellName(ghost_name)
    output = classify.Execute(dataset)

    assert output.HasGhostCellField()
    assert output.GetGhostCellFieldName() == ghost_name
    assert output.HasField(ghost_name, association="cells")

    ghost = output.GetGhostCellField()
    assert ghost.shape[0] == output.GetNumberOfCells()
    assert np.count_nonzero(ghost == 0) == required_num_normal_cells(nx, ny, nz, 1)


def main():
    cases = (
        (8, 0, 0),
        (5, 0, 0),
        (8, 4, 0),
        (5, 5, 0),
        (8, 8, 10),
        (5, 5, 5),
    )
    for nx, ny, nz in cases:
        run_case(nx, ny, nz, "MyGhostFieldName")


if __name__ == "__main__":
    main()
