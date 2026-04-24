##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.entity_extraction import GhostCellRemove
from viskores.testing import MakeGhostCellDataSet


def expected_cell_set_type(dataset_type, nz):
    if dataset_type == "explicit":
        return "explicit"
    return "structured2d" if nz == 0 else "structured3d"


def run_case(dataset_type, dims, layer, ghost_name, remove_mode):
    dataset = MakeGhostCellDataSet(dataset_type, dims, layer, ghost_name=ghost_name)
    assert dataset.HasGhostCellField()
    assert dataset.GetGhostCellField().shape[0] == dataset.GetNumberOfCells()

    ghost_cell_remove = GhostCellRemove()
    ghost_cell_remove.SetRemoveGhostField(True)

    if remove_mode == "all":
        ghost_cell_remove.SetTypesToRemoveToAll()
        assert ghost_cell_remove.AreAllTypesRemoved()
    else:
        ghost_cell_remove.SetTypesToRemove(1)
        assert not ghost_cell_remove.AreAllTypesRemoved()

    output = ghost_cell_remove.Execute(dataset)

    nx, ny, nz = dims
    expected_cells = (nx - (2 * layer)) * (ny - (2 * layer))
    if nz != 0:
        expected_cells *= nz - (2 * layer)

    assert output.GetNumberOfCells() == expected_cells
    assert output.CellSetTypeName() == expected_cell_set_type(dataset_type, nz)


def run_mid_ghost_case(dataset_type, dims, layer, ghost_name):
    dataset = MakeGhostCellDataSet(
        dataset_type, dims, layer, ghost_name=ghost_name, add_mid_ghost=True
    )
    ghost_cell_remove = GhostCellRemove()
    ghost_cell_remove.SetRemoveGhostField(True)
    output = ghost_cell_remove.Execute(dataset)
    assert output.CellSetTypeName() == "explicit"


def main():
    tests_2d = [(4, 4, 0, 2), (5, 5, 0, 2), (10, 10, 0, 3), (10, 5, 0, 2), (5, 10, 0, 2)]
    tests_3d = [(4, 4, 4, 2), (5, 5, 5, 2), (10, 10, 10, 3), (10, 5, 10, 2), (5, 10, 10, 2)]

    for nx, ny, nz, nghost in tests_2d + tests_3d:
        for layer in range(nghost):
            for dataset_type in ("uniform", "rectilinear", "explicit"):
                for ghost_name in ("default", "user-specified"):
                    for remove_mode in ("all", "byType"):
                        run_case(dataset_type, (nx, ny, nz), layer, ghost_name, remove_mode)

                    if dataset_type in ("uniform", "rectilinear"):
                        run_mid_ghost_case(dataset_type, (nx, ny, nz), layer, ghost_name)


if __name__ == "__main__":
    main()
