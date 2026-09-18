## Ray tracing supports 2D color textures

Surface ray tracing now supports color lookup in two-dimensional textures. A
texture field may contain either scalar values or two-component floating-point
values. Shape intersectors interpolate these values at each ray intersection,
normalize each component with its corresponding range, and store the resulting
coordinates in `Ray::TextureR` and `Ray::TextureS`.

Use `RayTracer::SetField` with an array containing one range per texture-field
component. Use `RayTracer::SetColorMap` with a flattened, row-major color array
and a `viskores::Id2` containing its width and height.

One-dimensional color lookup remains supported through the existing overloads.
It is represented as a texture with dimensions `(numberOfColors, 1)`, so the
S coordinate does not affect the selected color.
