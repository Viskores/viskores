##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.contour import Contour
from viskores.source import Tangle


def main():
    source = Tangle()
    source.SetCellDimensions((4, 4, 4))
    dataset = source.Execute()

    contour = Contour()
    contour.SetGenerateNormals(True)
    contour.SetIsoValue(0.5)
    contour.SetActiveField("tangle")
    result = contour.Execute(dataset)

    assert result.GetNumberOfPoints() == 72
    assert result.GetNumberOfCells() == 160


if __name__ == "__main__":
    main()
