## ANARI device includes the cylinder radius and instance transforms in the world bounds

The Viskores ANARI device computed the bounds of `cylinder` geometry from the
cylinder end points only, leaving out the radius, as it did for spheres. The
world's `bounds` property was then too small for thick cylinders, such as
the glyphs drawn by the ANARI glyphs mapper, and the cameras could place
their clipping range too close. Cylinder bounds now cover each end point
padded by the cylinder's `primitive.radius`, or by the `radius` parameter
when there is none.

The world bounds also left out instance transforms: each instance's group
bounds were added untransformed. The world bounds now cover the 8 corners
of each group's bounds after its instance's `transform`.
The device does not yet apply instance transforms when it renders, so this
changes only the reported bounds and the size of the cameras' default
clipping range, which follows the length of the bounds' diagonal.
