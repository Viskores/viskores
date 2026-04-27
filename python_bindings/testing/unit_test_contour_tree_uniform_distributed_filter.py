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

import viskores.cont
from viskores.filter.scalar_topology import ContourTreeUniformDistributed
from viskores.io import VTKDataSetReader
from viskores.testing import (
    CanonicalizeDistributedAugmentedTreeVolumes,
    CompileDistributedContourTreeSuperarcs,
    MakeTestDataSet,
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def read_edge_file(path: Path):
    edges = []
    for line in path.read_text(encoding="utf-8").splitlines():
        tokens = line.split()
        if len(tokens) == 2:
            edges.append((int(tokens[0]), int(tokens[1])))
    return edges


def run_distributed_filter(
    dataset,
    field_name,
    num_blocks,
    use_marching_cubes=False,
    augment_hierarchical_tree=False,
    pass_block_indices=True,
):
    partitions, blocks_per_dim, local_block_indices = viskores.cont.partition_uniform_dataset(
        dataset, field_name, num_blocks
    )
    contour_tree = ContourTreeUniformDistributed()
    contour_tree.SetActiveField(field_name)
    contour_tree.SetUseBoundaryExtremaOnly(True)
    contour_tree.SetUseMarchingCubes(use_marching_cubes)
    contour_tree.SetAugmentHierarchicalTree(augment_hierarchical_tree)
    if pass_block_indices:
        contour_tree.SetBlockIndices(blocks_per_dim[0], local_block_indices)
    return contour_tree.Execute(partitions)


def check_8x9():
    dataset = MakeTestDataSet().Make2DUniformDataSet3()
    expected = [
        (10, 20),
        (20, 34),
        (20, 38),
        (20, 61),
        (23, 34),
        (24, 34),
        (50, 61),
        (61, 71),
    ]
    for num_blocks in (8, 16):
        result = run_distributed_filter(dataset, "pointvar", num_blocks)
        assert CompileDistributedContourTreeSuperarcs(result) == expected


def check_5x6x7():
    dataset = MakeTestDataSet().Make3DUniformDataSet4()
    expected_freudenthal = [
        (0, 112),
        (71, 72),
        (72, 78),
        (72, 101),
        (101, 112),
        (101, 132),
        (107, 112),
        (131, 132),
        (132, 138),
    ]
    expected_marching_cubes = [
        (0, 203),
        (71, 72),
        (72, 78),
        (72, 101),
        (101, 112),
        (101, 132),
        (107, 112),
        (112, 203),
        (131, 132),
        (132, 138),
        (203, 209),
    ]
    for num_blocks in (8, 16):
        result = run_distributed_filter(dataset, "pointvar", num_blocks, use_marching_cubes=False)
        assert CompileDistributedContourTreeSuperarcs(result) == expected_freudenthal
        result = run_distributed_filter(dataset, "pointvar", num_blocks, use_marching_cubes=True)
        assert CompileDistributedContourTreeSuperarcs(result) == expected_marching_cubes


def check_vanc():
    root = repo_root()
    dataset = VTKDataSetReader(str(root / "data/data/rectilinear/vanc.vtk")).ReadDataSet()
    expected = read_edge_file(root / "data/baseline/vanc.ct_txt")
    for num_blocks in (8, 16):
        result = run_distributed_filter(dataset, "var", num_blocks)
        assert CompileDistributedContourTreeSuperarcs(result) == expected


def check_vanc_augmented_hierarchical_tree():
    root = repo_root()
    dataset = VTKDataSetReader(str(root / "data/data/rectilinear/vanc.vtk")).ReadDataSet()
    expected = (root / "data/baseline/vanc.augment_hierarchical_tree.ct_txt").read_text(
        encoding="utf-8"
    )
    for num_blocks, pass_block_indices in ((2, True), (4, True), (4, False)):
        result = run_distributed_filter(
            dataset,
            "var",
            num_blocks,
            augment_hierarchical_tree=True,
            pass_block_indices=pass_block_indices,
        )
        assert CanonicalizeDistributedAugmentedTreeVolumes(result, (21, 18, 1)) == expected


def main():
    viskores.cont.Initialize(["unit_test_contour_tree_uniform_distributed_filter.py"])
    check_8x9()
    check_5x6x7()
    check_vanc()
    check_vanc_augmented_hierarchical_tree()


if __name__ == "__main__":
    main()
