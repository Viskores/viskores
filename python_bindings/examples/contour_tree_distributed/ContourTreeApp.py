#!/usr/bin/env python3

##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import argparse
import sys
from pathlib import Path

import numpy as np

import viskores.cont
from viskores.cont import DataSet, create_uniform_dataset, partition_uniform_dataset
from viskores.filter.scalar_topology import (
    ContourTreeUniformDistributed,
    DistributedBranchDecompositionFilter,
    ExtractTopVolumeContoursFilter,
    SelectTopVolumeBranchesFilter,
)


def load_ascii_uniform_dataset(path: str) -> DataSet:
    with open(path, "r", encoding="utf-8") as stream:
        dims = tuple(int(x) for x in stream.readline().split())
        values = np.fromstring(stream.read(), sep=" ", dtype=np.float64)

    expected = int(np.prod(dims))
    if values.size != expected:
        raise ValueError(f"Expected {expected} values, got {values.size}")

    dataset = create_uniform_dataset(dims)
    dataset.AddPointField("values", values)
    return dataset


def main():
    parser = argparse.ArgumentParser(
        description="Python port of examples/contour_tree_distributed/ContourTreeApp.cxx using direct classes"
    )
    parser.add_argument("filename")
    parser.add_argument("--num-blocks", type=int, default=4)
    parser.add_argument("--mc", action="store_true")
    parser.add_argument("--use-full-boundary", action="store_true")
    parser.add_argument("--augment-hierarchical-tree", action="store_true")
    parser.add_argument("--compute-volume-branch-decomposition", action="store_true")
    parser.add_argument("--num-branches", type=int, default=0)
    parser.add_argument("--compute-isosurface", action="store_true")
    parser.add_argument("--shift-isovalue-by-epsilon", action="store_true")
    parser.add_argument("--presimplify-threshold", type=int, default=0)
    args, remaining_argv = parser.parse_known_args()

    viskores.cont.Initialize(
        [sys.argv[0], *remaining_argv], viskores.cont.InitializeOptions.RequireDevice
    )

    # Parse the command line options and the input data set.
    augment_hierarchical_tree = args.augment_hierarchical_tree
    if args.compute_volume_branch_decomposition or args.num_branches > 0:
        augment_hierarchical_tree = True

    dataset = load_ascii_uniform_dataset(args.filename)

    # Partition the data set to emulate the distributed workflow on one rank.
    partitions, blocks_per_dim, local_block_indices = partition_uniform_dataset(
        dataset, "values", args.num_blocks
    )

    # Compute the contour tree.
    contour_tree = ContourTreeUniformDistributed()
    contour_tree.SetActiveField("values")
    contour_tree.SetUseBoundaryExtremaOnly(not args.use_full_boundary)
    contour_tree.SetUseMarchingCubes(args.mc)
    contour_tree.SetAugmentHierarchicalTree(augment_hierarchical_tree)
    contour_tree.SetPresimplifyThreshold(args.presimplify_threshold)
    contour_tree.SetBlockIndices(blocks_per_dim[0], local_block_indices)
    current = contour_tree.Execute(partitions)

    # Compute the branch decomposition.
    if args.compute_volume_branch_decomposition or args.num_branches > 0:
        branch = DistributedBranchDecompositionFilter()
        current = branch.Execute(current)

    # Select the largest branches.
    if args.num_branches > 0:
        top = SelectTopVolumeBranchesFilter()
        top.SetSavedBranches(args.num_branches)
        top.SetPresimplifyThreshold(args.presimplify_threshold)
        current = top.Execute(current)
        if current.GetPartition(0).FieldNames() == []:
            print("warning: no valid branches remained after top-volume selection")
            print("skipping isosurface extraction")
            args.compute_isosurface = False
    iso_result = None

    # Extract the selected isosurfaces.
    if args.compute_isosurface and args.num_branches > 0:
        extract = ExtractTopVolumeContoursFilter()
        extract.SetMarchingCubes(args.mc)
        extract.SetShiftIsovalueByEpsilon(args.shift_isovalue_by_epsilon)
        iso_result = extract.Execute(current)

    print("blocks per dimension:", blocks_per_dim)
    print("local block indices shape:", local_block_indices.shape)
    print("partitions:", current.GetNumberOfPartitions())
    print("partition 0 fields:", current.GetPartition(0).FieldNames())

    base = Path(args.filename)
    if args.num_branches > 0:
        top0 = current.GetPartition(0)
        if top0.HasField("TopVolumeBranchSaddleIsoValue"):
            iso_values = top0.GetField("TopVolumeBranchSaddleIsoValue")
            out_path = base.with_suffix(".distributed.isovalues.txt")
            np.savetxt(out_path, iso_values.reshape(-1, 1))
            print("wrote:", out_path)

    # Write the extracted isosurface edges.
    if iso_result is not None:
        iso0 = iso_result.GetPartition(0)
        out_path = base.with_suffix(".distributed.isosurface_edges_from.txt")
        np.savetxt(out_path, iso0.GetField("IsosurfaceEdgesFrom"))
        print("wrote:", out_path)


if __name__ == "__main__":
    main()
