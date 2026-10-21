## ANARI device cameras, samplers and volumes use their defaults without parameters

The Viskores ANARI device only applies an object's parameters when one has
been set, so objects committed without any parameters kept uninitialized
members. A `perspective` camera committed without parameters could get a
zero field of view, which made rendering fail with "Camera field of view
must be greater than zero". Cameras, `image1D`/`image2D` samplers and
`transferFunction1D` volumes now start with the same defaults they use when
a parameter is missing:

  * Cameras: position (0, 0, 0), direction (0, 0, -1), up (0, 1, 0) and
    image region (0, 0) to (1, 1). A `perspective` camera has a field of
    view of pi/3, and an `orthographic` one an aspect and height of 1. Both
    pick their near and far planes from the scene bounds.
  * Samplers: identity input and output transforms, zero offsets and the
    `clampToEdge` wrap mode.
  * `transferFunction1D` volumes: color (1, 1, 1, 1), opacity 1 and unit
    distance 1.
