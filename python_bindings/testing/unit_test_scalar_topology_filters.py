##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from python_test_data import make_3d_uniform_dataset0
from viskores.cont import partition_uniform_dataset
from viskores.filter.scalar_topology import (
    ContourTreeAugmented,
    ContourTreeUniformDistributed,
    DistributedBranchDecompositionFilter,
    ExtractTopVolumeContoursFilter,
    SelectTopVolumeBranchesFilter,
)


def main():
    dataset = make_3d_uniform_dataset0()

    augmented = ContourTreeAugmented()
    augmented.SetActiveField("pointvar")
    result = augmented.Execute(dataset)
    assert result.GetNumberOfPoints() == dataset.GetNumberOfPoints()
    assert augmented.GetSortOrder().shape[0] == dataset.GetNumberOfPoints()
    assert augmented.GetSortedSuperarcs().shape[1] == 2
    assert int(augmented.GetNumIterations()) > 0

    partitions, blocks_per_dim, local_block_indices = partition_uniform_dataset(
        dataset, "pointvar", 2
    )

    distributed = ContourTreeUniformDistributed()
    distributed.SetActiveField("pointvar")
    distributed.SetAugmentHierarchicalTree(True)
    distributed.SetBlockIndices(blocks_per_dim[0], local_block_indices)
    distributed_result = distributed.Execute(partitions)
    assert distributed_result.GetNumberOfPartitions() == partitions.GetNumberOfPartitions()

    branch = DistributedBranchDecompositionFilter()
    branch_result = branch.Execute(distributed_result)
    assert branch_result.GetNumberOfPartitions() == distributed_result.GetNumberOfPartitions()

    top = SelectTopVolumeBranchesFilter()
    top.SetSavedBranches(1)
    top_result = top.Execute(branch_result)
    assert top_result.GetNumberOfPartitions() == branch_result.GetNumberOfPartitions()

    extract = ExtractTopVolumeContoursFilter()
    top0 = top_result.GetPartition(0)
    if top0.FieldNames() != []:
        contour_result = extract.Execute(top_result)
        assert contour_result.GetNumberOfPartitions() == top_result.GetNumberOfPartitions()


if __name__ == "__main__":
    main()
