from viskores import Range
from viskores.cont import ColorTable


def main():
    color_table = ColorTable("Inferno")
    assert color_table.GetName() == "Inferno"
    assert color_table.GetNumberOfPoints() > 0

    color_table.SetClampingOff()
    assert not color_table.GetClamping()
    color_table.SetClampingOn()
    assert color_table.GetClamping()

    color_table.SetBelowRangeColor((0.0, 0.0, 1.0))
    color_table.SetAboveRangeColor((1.0, 0.0, 0.0))
    color_table.RescaleToRange(Range(-1.0, 2.0))
    assert color_table.GetRange() == Range(-1.0, 2.0)
    color_table.AddPointAlpha(0.5, 0.75)


if __name__ == "__main__":
    main()
