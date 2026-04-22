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

from viskores.io import BOVDataSetReader


def main():
    repo_root = Path(__file__).resolve().parents[2]
    bov_file = repo_root / "data" / "data" / "third_party" / "visit" / "example_temp.bov"

    dataset = BOVDataSetReader(str(bov_file)).ReadDataSet()

    assert len(dataset.FieldNames()) == 2
    assert dataset.GetNumberOfPoints() == 50 * 50 * 50
    assert dataset.GetNumberOfCells() == 49 * 49 * 49
    assert dataset.HasField("var", association="points")


if __name__ == "__main__":
    main()
