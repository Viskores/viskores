## ANARI device includes the cylinder radius in the world bounds

The Viskores ANARI device computed the bounds of `cylinder` geometry from the
cylinder end points only, leaving out the radius, as it did for spheres. The
world's `bounds` property was then too small for thick cylinders, such as
the glyphs drawn by the ANARI glyphs mapper, and the cameras could place
their clipping range too close. Cylinder bounds now cover each end point
padded by the cylinder's `primitive.radius`, or by the `radius` parameter
when there is none.
