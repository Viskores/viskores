##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from pathlib import Path

import numpy as np

import viskores.cont
from viskores.filter.scalar_topology import (
    ContourTreeUniformDistributed,
    DistributedBranchDecompositionFilter,
    ExtractTopVolumeContoursFilter,
    SelectTopVolumeBranchesFilter,
)
from viskores.io import VTKDataSetReader
from viskores.testing import CanonicalizeDistributedBranchDecomposition, MakeTestDataSet


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def run_branch_pipeline(dataset, field_name, num_blocks):
    partitions, blocks_per_dim, local_block_indices = viskores.cont.partition_uniform_dataset(
        dataset, field_name, num_blocks
    )
    contour_tree = ContourTreeUniformDistributed()
    contour_tree.SetActiveField(field_name)
    contour_tree.SetUseBoundaryExtremaOnly(True)
    contour_tree.SetAugmentHierarchicalTree(True)
    contour_tree.SetBlockIndices(blocks_per_dim[0], local_block_indices)
    distributed = contour_tree.Execute(partitions)

    branch = DistributedBranchDecompositionFilter()
    branch_result = branch.Execute(distributed)

    top = SelectTopVolumeBranchesFilter()
    top.SetSavedBranches(2)
    top_result = top.Execute(branch_result)

    iso = ExtractTopVolumeContoursFilter()
    iso.SetMarchingCubes(False)
    iso_result = iso.Execute(top_result)
    return branch_result, top_result, iso_result


def unique_branch_edges(branch_result):
    computed = set()
    for partition_index in range(branch_result.GetNumberOfPartitions()):
        ds = branch_result.GetPartition(partition_index)
        upper = ds.GetField("UpperEndGlobalRegularIds")
        lower = ds.GetField("LowerEndGlobalRegularIds")
        for edge_index in range(upper.shape[0]):
            computed.add((int(upper[edge_index]), int(lower[edge_index])))
    return sorted(computed)


def check_8x9_branch_decomposition():
    dataset = MakeTestDataSet().Make2DUniformDataSet3()
    expected_edges = sorted([(10, 20), (23, 71), (34, 24), (38, 20), (61, 50)])
    expected_branch0 = (38, 6, 1, 50)
    expected_branch1 = (50, 2, -1, 30)

    for num_blocks in (2, 4, 8, 16):
        branch_result, top_result, iso_result = run_branch_pipeline(dataset, "pointvar", num_blocks)
        assert unique_branch_edges(branch_result) == expected_edges

        for partition_index in range(top_result.GetNumberOfPartitions()):
            ds = top_result.GetPartition(partition_index)
            gr_id = ds.GetField("TopVolumeBranchGlobalRegularIds")
            volume = ds.GetField("TopVolumeBranchVolume")
            epsilon = ds.GetField("TopVolumeBranchSaddleEpsilon")
            iso_value = ds.GetField("TopVolumeBranchSaddleIsoValue")
            assert tuple(int(x) for x in (gr_id[0], volume[0], epsilon[0], iso_value[0])) == expected_branch0
            assert tuple(int(x) for x in (gr_id[1], volume[1], epsilon[1], iso_value[1])) == expected_branch1

        if num_blocks != 2:
            continue

        expected_info = {
            0: [5, 1, 50, 4, 0, 50],
            1: [1, 2, 30, 4, 0, 50],
        }
        expected_first_from = {
            0: np.array((0.519231, 3.0, 0.0)),
            1: np.array((4.33333, 5.0, 0.0)),
        }
        expected_first_to = {
            0: np.array((0.5, 2.5, 0.0)),
            1: np.array((4.61538, 4.61538, 0.0)),
        }
        expected_counts = {0: 25, 1: 26}

        for partition_index in range(iso_result.GetNumberOfPartitions()):
            ds = iso_result.GetPartition(partition_index)
            edges_from = ds.GetField("IsosurfaceEdgesFrom")
            edges_to = ds.GetField("IsosurfaceEdgesTo")
            labels = ds.GetField("IsosurfaceEdgesLabels")
            orders = ds.GetField("IsosurfaceEdgesOrders")
            offsets = ds.GetField("IsosurfaceEdgesOffset")
            iso_values = ds.GetField("IsosurfaceIsoValue")

            computed_info = []
            iso_surface_count = 0
            for edge_index in range(edges_from.shape[0]):
                while iso_surface_count < labels.shape[0] and edge_index == int(offsets[iso_surface_count]):
                    computed_info.extend(
                        (int(labels[iso_surface_count]), int(orders[iso_surface_count]), int(iso_values[iso_surface_count]))
                    )
                    iso_surface_count += 1

            assert iso_surface_count == 2
            assert computed_info == expected_info[partition_index]
            assert edges_from.shape[0] == expected_counts[partition_index]
            assert np.allclose(edges_from[0], expected_first_from[partition_index])
            assert np.allclose(edges_to[0], expected_first_to[partition_index])


def check_vanc_branch_compile():
    root = repo_root()
    dataset = VTKDataSetReader(str(root / "data/data/rectilinear/vanc.vtk")).ReadDataSet()
    expected = (root / "data/baseline/vanc.branch_compile.ct_txt").read_text(encoding="utf-8")

    for num_blocks in (2, 4, 8, 16):
        branch_result, _, _ = run_branch_pipeline(dataset, "var", num_blocks)
        assert CanonicalizeDistributedBranchDecomposition(branch_result) == expected


def main():
    viskores.cont.Initialize(["unit_test_distributed_branch_decomposition_filter.py"])
    check_8x9_branch_decomposition()
    check_vanc_branch_compile()


if __name__ == "__main__":
    main()
