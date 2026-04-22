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
from viskores.rendering import MapperRayTracer
from viskores.source import PerlinNoise

from render_test_utils import render_with_mapper


def main():
    noise_source = PerlinNoise()
    noise_source.SetCellDimensions((16, 16, 16))
    noise_source.SetSeed(77698)
    noise = noise_source.Execute()

    contour = Contour()
    contour.SetIsoValues((0.3, 0.4, 0.5, 0.6, 0.7))
    contour.SetActiveField("perlinnoise")
    contours = contour.Execute(noise)

    render_with_mapper(contours, "perlinnoise", MapperRayTracer(), "perlin_noise.png")


if __name__ == "__main__":
    main()
