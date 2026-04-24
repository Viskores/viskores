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
from viskores.cont import DataSet, create_uniform_dataset
from viskores.filter.scalar_topology import ContourTreeAugmented


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
        description="Python port of examples/contour_tree_augmented/ContourTreeApp.cxx using direct classes"
    )
    parser.add_argument("filename")
    parser.add_argument("--mc", action="store_true")
    parser.add_argument("--augment-tree", type=int, default=1)
    parser.add_argument("--branch-decomp", action="store_true")
    parser.add_argument("--levels", type=int, default=0)
    parser.add_argument("--type", type=int, default=0, dest="contour_type")
    parser.add_argument("--eps", type=float, default=1.0e-5)
    parser.add_argument("--comp", type=int, default=0)
    parser.add_argument("--method", type=int, default=0)
    parser.add_argument("--use-volume-sorter", action="store_true")
    args, remaining_argv = parser.parse_known_args()

    viskores.cont.Initialize(
        [sys.argv[0], *remaining_argv], viskores.cont.InitializeOptions.RequireDevice
    )

    # Parse the input data set.
    dataset = load_ascii_uniform_dataset(args.filename)

    compute_branch_decomposition = args.branch_decomp or args.levels > 0
    if compute_branch_decomposition and args.augment_tree == 0:
        print(
            "warning: regular structure is required for branch decomposition. "
            "disabling branch decomposition"
        )
        compute_branch_decomposition = False

    # Convert the mesh of values into a contour tree.
    contour_tree = ContourTreeAugmented(args.mc, args.augment_tree)
    contour_tree.SetActiveField("values")
    contour_tree.Execute(dataset)

    # Dump out the contour tree for comparison.
    sorted_superarcs = contour_tree.GetSortedSuperarcs()

    print("sorted superarcs:", sorted_superarcs.shape[0])
    print("num iterations:", contour_tree.GetNumIterations())

    # Compute the branch decomposition.
    branch_decomposition = None
    if compute_branch_decomposition and args.augment_tree:
        branch_decomposition = contour_tree.ComputeVolumeBranchDecomposition()
        print("branches:", branch_decomposition["branch_parent"].shape[0])

    # Compute the relevant isovalues from the explicit branch decomposition.
    if args.levels > 0 and compute_branch_decomposition and args.augment_tree:
        iso_values = contour_tree.ComputeRelevantValues(
            dataset,
            args.levels,
            contour_type=args.contour_type,
            eps=args.eps,
            num_components=args.comp,
            contour_select_method=args.method,
            use_persistence_sorter=not args.use_volume_sorter,
        )
        print("isovalues:", " ".join(str(v) for v in iso_values))

    # Write the sorted superarc list.
    out_path = Path(args.filename).with_suffix(".augmented.sorted_superarcs.txt")
    np.savetxt(out_path, sorted_superarcs, fmt="%d")
    print("wrote:", out_path)


if __name__ == "__main__":
    main()
