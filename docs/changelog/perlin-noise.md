# Perlin Noise source

A new source, `vtkm::source::PerlinNoise`, has been added. As the name
would imply, this source generates a pseudo-random [Perlin
noise](https://en.wikipedia.org/wiki/Perlin_noise) field.

The field is defined on a 3D grid of specified dimensions. A seed value can
also be specified to enforce consistent results in, for example, test code.
If a seed is not specified, one will be created based on the current system
time.

Perlin noise is useful for testing purposes as it can create non-trivial
geometry at pretty much any scale.
