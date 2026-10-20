## ANARI device matte material uses its defaults without parameters

The Viskores ANARI device read uninitialized memory when a `matte` material
was committed without any parameters set. The device only applies a
material's parameters when one has been set, so the color, opacity and alpha
mode of such a material were never given values. Surfaces using it could be
drawn with an arbitrary color or opacity, or not at all, and Valgrind
reported the uninitialized reads in `Canvas::BlendBackground`. The material
now starts with the default color (0.8, 0.8, 0.8), opacity 1, opaque alpha
mode and alpha cutoff 0.5.
