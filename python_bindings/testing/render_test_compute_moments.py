##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import viskores.cont
from viskores.filter.image_processing import ComputeMoments
from viskores.rendering import MapperRayTracer
from viskores.source import Wavelet

from render_test_utils import render_with_mapper


def main():
    viskores.cont.Initialize(["render_test_compute_moments.py"])

    moments = ComputeMoments()
    moments.SetActiveField("RTData")
    moments.SetOrder(2)
    moments.SetRadius(2)
    result = moments.Execute(Wavelet().Execute())

    for field_name, output_name in (
        ("index", "moments.png"),
        ("index0", "moments0.png"),
        ("index12", "moments12.png"),
    ):
        assert result.HasField(field_name)
        render_with_mapper(result, field_name, MapperRayTracer(), output_name)


if __name__ == "__main__":
    main()
