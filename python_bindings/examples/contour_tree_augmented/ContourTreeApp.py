#!/usr/bin/env python3

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
        description="Python port of examples/contour_tree_augmented/ContourTreeApp.cxx"
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
    args = parser.parse_args()

    dataset = load_ascii_uniform_dataset(args.filename)
    result = viskores.contour_tree_augmented(
        dataset,
        "values",
        use_marching_cubes=args.mc,
        compute_regular_structure=args.augment_tree,
        compute_branch_decomposition=args.branch_decomp or args.levels > 0,
        num_levels=args.levels,
        contour_type=args.contour_type,
        eps=args.eps,
        num_components=args.comp,
        contour_select_method=args.method,
        use_persistence_sorter=not args.use_volume_sorter,
    )

    print("sorted superarcs:", result["sorted_superarcs"].shape[0])
    print("num iterations:", result["num_iterations"])
    if "branch_parent" in result:
        print("branches:", result["branch_parent"].shape[0])
    if "isovalues" in result:
        print("isovalues:", " ".join(str(v) for v in result["isovalues"]))

    out_path = Path(args.filename).with_suffix(".augmented.sorted_superarcs.txt")
    np.savetxt(out_path, result["sorted_superarcs"], fmt="%d")
    print("wrote:", out_path)


if __name__ == "__main__":
    main()
