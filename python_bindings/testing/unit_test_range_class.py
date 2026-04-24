import math

import viskores


def main():
    empty_range = viskores.Range()
    assert not empty_range.IsNonEmpty()
    assert empty_range.Length() == 0.0

    single_value_range = viskores.Range(5.0, 5.0)
    assert single_value_range.IsNonEmpty()
    assert single_value_range.Length() == 0.0
    assert single_value_range.Center() == 5.0
    assert single_value_range.Contains(5.0)
    assert not single_value_range.Contains(0.0)

    low_range = viskores.Range(-10.0, -5.0)
    union_range = single_value_range + low_range
    assert union_range.IsNonEmpty()
    assert math.isclose(union_range.Length(), 15.0)
    assert math.isclose(union_range.Center(), -2.5)
    assert union_range.Contains(-7.0)
    assert union_range.Contains(0.0)
    assert not union_range.Contains(10.0)

    high_range = viskores.Range(15.0, 20.0)
    union_range = high_range.Union(single_value_range)
    assert union_range == viskores.Range(5.0, 20.0)
    assert union_range != single_value_range

    union_range.Include(-1.0)
    assert union_range == viskores.Range(-1.0, 20.0)

    union_range.Include(low_range)
    assert union_range == viskores.Range(-10.0, 20.0)
    assert math.isclose(union_range.Center(), 5.0)

    simple_range = viskores.Range(2.0, 4.0)
    simple_range.Min = 1.0
    simple_range.Max = 2.0
    assert simple_range.Contains(1.5)
    assert not simple_range.Contains(3.0)


if __name__ == "__main__":
    main()
