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
from viskores.cont import (
    DataSetBuilderExplicit,
    DataSetBuilderRectilinear,
    DataSetBuilderUniform,
)
from viskores.filter.entity_extraction import GhostCellRemove
from viskores.python_convenience import cell_set_type_name


def make_structured_ghost_cell_array(nx, ny, nz, num_layers, add_mid_ghost=False):
    shape = (ny, nx) if nz == 0 else (nz, ny, nx)
    ghosts = np.full(shape, 1 if num_layers else 0, dtype=np.uint8)

    if num_layers > 0:
        if nz == 0:
            ghosts[num_layers : ny - num_layers, num_layers : nx - num_layers] = 0
        else:
            ghosts[
                num_layers : nz - num_layers,
                num_layers : ny - num_layers,
                num_layers : nx - num_layers,
            ] = 0

    if add_mid_ghost:
        mi = num_layers + (nx - num_layers) // 2
        mj = num_layers + (ny - num_layers) // 2
        if nz == 0:
            ghosts[mj, mi] = 1
        else:
            mk = num_layers + (nz - num_layers) // 2
            ghosts[mk, mj, mi] = 1

    return ghosts.reshape(-1)


def structured_point_coordinates(nx, ny, nz):
    if nz == 0:
        return np.array(
            [(i, j, 0.0) for j in range(ny + 1) for i in range(nx + 1)],
            dtype=np.float32,
        )
    return np.array(
        [
            (i, j, k)
            for k in range(nz + 1)
            for j in range(ny + 1)
            for i in range(nx + 1)
        ],
        dtype=np.float32,
    )


def explicit_connectivity(nx, ny, nz):
    if nz == 0:
        shapes = np.full(nx * ny, viskores.CELL_SHAPE_QUAD, dtype=np.uint8)
        num_indices = np.full(nx * ny, 4, dtype=np.int32)
        connectivity = []
        for j in range(ny):
            for i in range(nx):
                p0 = (j * (nx + 1)) + i
                connectivity.extend((p0, p0 + 1, p0 + nx + 2, p0 + nx + 1))
        return shapes, num_indices, np.asarray(connectivity, dtype=np.int64)

    shapes = np.full(nx * ny * nz, viskores.CELL_SHAPE_HEXAHEDRON, dtype=np.uint8)
    num_indices = np.full(nx * ny * nz, 8, dtype=np.int32)

    def point_id(i, j, k):
        return (k * (ny + 1) * (nx + 1)) + (j * (nx + 1)) + i

    connectivity = []
    for k in range(nz):
        for j in range(ny):
            for i in range(nx):
                connectivity.extend(
                    (
                        point_id(i, j, k),
                        point_id(i + 1, j, k),
                        point_id(i + 1, j + 1, k),
                        point_id(i, j + 1, k),
                        point_id(i, j, k + 1),
                        point_id(i + 1, j, k + 1),
                        point_id(i + 1, j + 1, k + 1),
                        point_id(i, j + 1, k + 1),
                    )
                )
    return shapes, num_indices, np.asarray(connectivity, dtype=np.int64)


def make_ghost_cell_dataset(
    dataset_type, dims, ghost_layers, ghost_name="default", add_mid_ghost=False
):
    nx, ny, nz = dims
    point_dims = (nx + 1, ny + 1) if nz == 0 else (nx + 1, ny + 1, nz + 1)

    if dataset_type == "uniform":
        dataset = DataSetBuilderUniform.Create(point_dims)
    elif dataset_type == "rectilinear":
        axes = [np.arange(size, dtype=np.float32) for size in point_dims]
        dataset = DataSetBuilderRectilinear.Create(*axes)
    elif dataset_type == "explicit":
        coords = structured_point_coordinates(nx, ny, nz)
        shapes, num_indices, connectivity = explicit_connectivity(nx, ny, nz)
        dataset = DataSetBuilderExplicit.Create(coords, shapes, num_indices, connectivity)
    else:
        raise ValueError("dataset_type must be one of: uniform, rectilinear, explicit.")

    ghosts = make_structured_ghost_cell_array(nx, ny, nz, ghost_layers, add_mid_ghost)
    if ghost_name == "default":
        dataset.SetGhostCellField(ghosts)
    else:
        dataset.SetGhostCellField(ghost_name, ghosts)
    return dataset


def expected_cell_set_type(dataset_type, nz):
    if dataset_type == "explicit":
        return "explicit"
    return "structured2d" if nz == 0 else "structured3d"


def run_case(dataset_type, dims, layer, ghost_name, remove_mode):
    dataset = make_ghost_cell_dataset(dataset_type, dims, layer, ghost_name=ghost_name)
    assert dataset.HasGhostCellField()
    assert dataset.GetGhostCellField().GetData().AsNumPy().shape[0] == dataset.GetNumberOfCells()

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
    assert cell_set_type_name(output) == expected_cell_set_type(dataset_type, nz)


def run_mid_ghost_case(dataset_type, dims, layer, ghost_name):
    dataset = make_ghost_cell_dataset(
        dataset_type, dims, layer, ghost_name=ghost_name, add_mid_ghost=True
    )
    ghost_cell_remove = GhostCellRemove()
    ghost_cell_remove.SetRemoveGhostField(True)
    output = ghost_cell_remove.Execute(dataset)
    assert cell_set_type_name(output) == "explicit"


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
