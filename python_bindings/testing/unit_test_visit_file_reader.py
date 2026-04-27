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

from viskores.io import VTKVisItFileReader


def main():
    repo_root = Path(__file__).resolve().parents[2]
    visit_file = repo_root / "data" / "data" / "uniform" / "venn250.visit"

    partitioned = VTKVisItFileReader(str(visit_file)).ReadPartitionedDataSet()

    assert partitioned.GetNumberOfPartitions() == 2
    for index in range(partitioned.GetNumberOfPartitions()):
        dataset = partitioned.GetPartition(index)
        assert dataset.GetNumberOfPoints() == 63001
        assert len(dataset.FieldNames()) == 5


if __name__ == "__main__":
    main()
