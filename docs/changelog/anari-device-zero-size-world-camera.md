## ANARI device cameras handle a world of zero size

The Viskores ANARI device places the camera's look-at point and clipping
range using the size of the world's bounds, which come from the geometry's
coordinates. For a world of zero size, such as a single sphere, the look-at
point landed on the camera position and the clipping range collapsed, so
nothing was rendered. Perspective and orthographic cameras now use a unit
length in that case.
