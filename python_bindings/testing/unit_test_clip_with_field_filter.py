##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.filter.contour import ClipWithField
from viskores.testing import MakeTestDataSet


def main():
    dataset = MakeTestDataSet().Make3DUniformDataSet3((10, 10, 10))

    clip = ClipWithField()
    clip.SetClipValue(0.0)
    clip.SetActiveField("pointvar")
    clip.SetFieldsToPass("pointvar")
    output = clip.Execute(dataset)

    assert output.GetNumberOfCoordinateSystems() == 1
    assert output.GetNumberOfFields() >= 2
    assert output.HasField("pointvar")
    assert output.GetField("pointvar").size > 0


if __name__ == "__main__":
    main()
