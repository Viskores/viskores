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
from pathlib import Path

import numpy as np

import viskores


def load_ascii_uniform_dataset(path: str) -> viskores.DataSet:
    with open(path, "r", encoding="utf-8") as stream:
        dims = tuple(int(x) for x in stream.readline().split())
        values = np.fromstring(stream.read(), sep=" ", dtype=np.float64)

    expected = int(np.prod(dims))
    if values.size != expected:
        raise ValueError(f"Expected {expected} values, got {values.size}")

    dataset = viskores.create_uniform_dataset(dims)
    dataset.add_point_field("values", values)
    return dataset


def main():
    parser = argparse.ArgumentParser(
        description="Python port of examples/contour_tree_distributed/ContourTreeApp.cxx"
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
    args = parser.parse_args()

    augment_hierarchical_tree = args.augment_hierarchical_tree
    if args.compute_volume_branch_decomposition or args.num_branches > 0:
        augment_hierarchical_tree = True

    dataset = load_ascii_uniform_dataset(args.filename)
    partitions, blocks_per_dim, local_block_indices = viskores.partition_uniform_dataset(
        dataset, "values", args.num_blocks
    )

    result = viskores.contour_tree_distributed(
        partitions,
        "values",
        blocks_per_dim,
        local_block_indices,
        use_boundary_extrema_only=not args.use_full_boundary,
        use_marching_cubes=args.mc,
        augment_hierarchical_tree=augment_hierarchical_tree,
        presimplify_threshold=args.presimplify_threshold,
    )

    current = result
    if args.compute_volume_branch_decomposition or args.num_branches > 0:
        current = viskores.distributed_branch_decomposition(current)
    if args.num_branches > 0:
        current = viskores.select_top_volume_branches(
            current,
            args.num_branches,
            presimplify_threshold=args.presimplify_threshold,
        )
        if current.get_partition(0).field_names() == []:
            print("warning: no valid branches remained after top-volume selection")
            print("skipping isosurface extraction")
            args.compute_isosurface = False
    iso_result = None
    if args.compute_isosurface and args.num_branches > 0:
        iso_result = viskores.extract_top_volume_contours(
            current,
            marching_cubes=args.mc,
            shift_isovalue_by_epsilon=args.shift_isovalue_by_epsilon,
        )

    print("blocks per dimension:", blocks_per_dim)
    print("local block indices shape:", local_block_indices.shape)
    print("partitions:", current.number_of_partitions)
    print("partition 0 fields:", current.get_partition(0).field_names())

    base = Path(args.filename)
    if args.num_branches > 0:
        top = current.get_partition(0)
        if top.has_field("TopVolumeBranchSaddleIsoValue"):
            iso_values = top.get_field("TopVolumeBranchSaddleIsoValue")
            out_path = base.with_suffix(".distributed.isovalues.txt")
            np.savetxt(out_path, iso_values.reshape(-1, 1))
            print("wrote:", out_path)

    if iso_result is not None:
        iso0 = iso_result.get_partition(0)
        out_path = base.with_suffix(".distributed.isosurface_edges_from.txt")
        np.savetxt(out_path, iso0.get_field("IsosurfaceEdgesFrom"))
        print("wrote:", out_path)


if __name__ == "__main__":
    main()
