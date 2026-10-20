## ANARI device includes the sphere radius in the world bounds

The Viskores ANARI device computed the bounds of `sphere` geometry from the
sphere centers only, leaving out the radius. The world's `bounds` property
was then too small, and zero for a single sphere. The cameras place their
clipping range from these bounds, so spheres that were large compared with
the spread of their centers could fall behind the far plane and not be
rendered. Sphere bounds now cover each whole sphere, using the per-vertex
radius where given and the `radius` parameter otherwise.
