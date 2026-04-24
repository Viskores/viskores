##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from viskores.cont import ColorTable
from viskores.rendering import MapperVolume
from viskores.source import Tangle

from render_test_utils import render_with_mapper


def main():
    source = Tangle()
    source.SetPointDimensions((50, 50, 50))
    dataset = source.Execute()

    color_table = ColorTable("inferno")
    color_table.AddPointAlpha(0.0, 0.2)
    color_table.AddPointAlpha(0.2, 0.0)
    color_table.AddPointAlpha(0.5, 0.0)

    mapper = MapperVolume()
    mapper.SetCompositeBackground(False)
    render_with_mapper(dataset, "tangle", mapper, "mapper_volume.png")


if __name__ == "__main__":
    main()
