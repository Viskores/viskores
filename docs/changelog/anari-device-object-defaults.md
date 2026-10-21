## ANARI device cameras, samplers and volumes use their defaults without parameters

The Viskores ANARI device only applies an object's parameters when one has
been set, so objects committed without any parameters kept uninitialized
members. A `perspective` camera committed without parameters had an
arbitrary field of view, which could make rendering fail with "Camera field
of view must be greater than zero", and an `orthographic` one had an
arbitrary position and direction. Cameras, `image1D`/`image2D` samplers and
`transferFunction1D` volumes now start with the same defaults they use when
a parameter is missing.
