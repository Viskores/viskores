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


NO_SUCH_ELEMENT = -(1 << 63)
INDEX_MASK = (1 << 59) - 1
IS_ASCENDING = 1 << 59


def _field(dataset, name, dtype=None):
    array = np.asarray(dataset.GetField(name))
    if dtype is None:
        return array
    return array.astype(dtype, copy=False)


def _no_such_element(value):
    return (int(value) & NO_SUCH_ELEMENT) != 0


def _is_ascending(value):
    return (int(value) & IS_ASCENDING) != 0


def _masked_index(value):
    return int(value) & INDEX_MASK


def compile_distributed_contour_tree_superarcs(partitioned_dataset):
    supernodes_on_superarcs = []

    for partition_index in range(partitioned_dataset.GetNumberOfPartitions()):
        dataset = partitioned_dataset.GetPartition(partition_index)
        data_values = _field(dataset, "DataValues")
        global_regular_ids = _field(dataset, "RegularNodeGlobalIds", np.int64)
        superarcs = _field(dataset, "Superarcs", np.int64)
        supernodes = _field(dataset, "Supernodes", np.int64)
        superparents = _field(dataset, "Superparents", np.int64)

        for supernode, regular_id_value in enumerate(supernodes):
            regular_id = int(regular_id_value)
            global_id = int(global_regular_ids[regular_id])
            data_value = float(data_values[regular_id])
            super_to = int(superarcs[supernode])

            if _no_such_element(super_to):
                superparent = int(superparents[regular_id])
                if superparent == supernode:
                    continue

                regular_from = int(supernodes[superparent])
                global_from = int(global_regular_ids[regular_from])
                super_parent_to = int(superarcs[superparent])
                regular_to = int(supernodes[_masked_index(super_parent_to)])
                global_to = int(global_regular_ids[regular_to])
                low_end, high_end = (
                    (global_from, global_to)
                    if _is_ascending(super_parent_to)
                    else (global_to, global_from)
                )
                supernodes_on_superarcs.append((low_end, high_end, data_value, global_id))
                continue

            regular_to = int(supernodes[_masked_index(super_to)])
            global_to = int(global_regular_ids[regular_to])
            data_to = float(data_values[regular_to])

            if _is_ascending(super_to):
                low_end, high_end = global_id, global_to
            else:
                low_end, high_end = global_to, global_id

            supernodes_on_superarcs.append((low_end, high_end, data_value, global_id))
            supernodes_on_superarcs.append((low_end, high_end, data_to, global_to))

    supernodes_on_superarcs.sort()
    edges = []
    for current, next_node in zip(supernodes_on_superarcs, supernodes_on_superarcs[1:]):
        if current[0] != next_node[0] or current[1] != next_node[1]:
            continue
        if current[3] == next_node[3]:
            continue
        edges.append((min(current[3], next_node[3]), max(current[3], next_node[3])))

    return sorted(edges)


def _branch_lines(triples):
    current_branch = None
    high_value = low_value = 0.0
    high_end = low_end = NO_SUCH_ELEMENT
    branches = []

    for branch, value, supernode in sorted(set(triples)):
        if branch != current_branch:
            if current_branch is not None:
                branches.append((high_end, low_end))
            current_branch = branch
            high_value = low_value = value
            high_end = low_end = supernode
            continue

        if value > high_value or (value == high_value and supernode > high_end):
            high_value = value
            high_end = supernode
        elif value < low_value or (value == low_value and supernode < low_end):
            low_value = value
            low_end = supernode

    if current_branch is not None:
        branches.append((high_end, low_end))

    return "".join(f"{high:12d}{low:14d}\n" for high, low in sorted(branches))


def canonicalize_distributed_branch_decomposition(partitioned_dataset):
    triples = []

    for partition_index in range(partitioned_dataset.GetNumberOfPartitions()):
        dataset = partitioned_dataset.GetPartition(partition_index)
        data_values = _field(dataset, "DataValues")
        global_regular_ids = _field(dataset, "RegularNodeGlobalIds", np.int64)
        superarcs = _field(dataset, "Superarcs", np.int64)
        supernodes = _field(dataset, "Supernodes", np.int64)
        branch_roots = _field(dataset, "BranchRoots", np.int64)

        for superarc, target in enumerate(superarcs):
            if _no_such_element(target):
                continue

            branch_root_super_id = int(branch_roots[superarc])
            branch_root_regular_id = int(supernodes[branch_root_super_id])
            branch_root_global_id = int(global_regular_ids[branch_root_regular_id])

            from_regular_id = int(supernodes[superarc])
            triples.append(
                (
                    branch_root_global_id,
                    float(data_values[from_regular_id]),
                    int(global_regular_ids[from_regular_id]),
                )
            )

            to_regular_id = _masked_index(target)
            triples.append(
                (
                    branch_root_global_id,
                    float(data_values[to_regular_id]),
                    int(global_regular_ids[to_regular_id]),
                )
            )

    return _branch_lines(triples)


def canonicalize_distributed_augmented_tree_volumes(partitioned_dataset, global_size):
    total_volume = int(np.prod(global_size, dtype=np.int64))
    volumes = set()

    for partition_index in range(partitioned_dataset.GetNumberOfPartitions()):
        dataset = partitioned_dataset.GetPartition(partition_index)
        global_regular_ids = _field(dataset, "RegularNodeGlobalIds", np.int64)
        superarcs = _field(dataset, "Superarcs", np.int64)
        supernodes = _field(dataset, "Supernodes", np.int64)
        intrinsic_volume = _field(dataset, "IntrinsicVolume", np.int64)
        dependent_volume = _field(dataset, "DependentVolume", np.int64)

        for supernode, target in enumerate(superarcs):
            if _no_such_element(target):
                continue

            from_global = int(global_regular_ids[int(supernodes[supernode])])
            to_super = _masked_index(target)
            to_global = int(global_regular_ids[int(supernodes[to_super])])
            weight = int(dependent_volume[supernode])
            arc_weight = int(intrinsic_volume[supernode]) - 1
            counter_weight = total_volume - weight + arc_weight

            if _is_ascending(target):
                volumes.add((to_global, from_global, weight, arc_weight, counter_weight))
            else:
                volumes.add((from_global, to_global, counter_weight, arc_weight, weight))

    lines = ["============\n", "Contour Tree\n"]
    lines.extend(
        f"H: {high:8d} L: {low:8d} VH: {volume_high:8d} "
        f"VR: {volume_regular:8d} VL: {volume_low:8d}\n"
        for high, low, volume_high, volume_regular, volume_low in sorted(volumes)
    )
    return "".join(lines)
