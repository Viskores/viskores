## ANARI glyphs handle zero vectors, unsupported fields and single points

`ANARIMapperGlyphs` used to produce NaN cone vertices for a zero-length
vector, which can corrupt or crash some ANARI devices. Zero and non-finite
vectors now get a zero-size glyph at their point. Very short or very long
vectors, whose squared length would underflow or overflow, now get a normal
glyph. A field that does not have 3 components, or is not a point or cell
field with one value per point or cell, used to throw from
`GetANARISurface()`. The mapper now logs a warning and leaves its group
empty. When the coordinate bounds have zero size, such as a single point, the
glyph size falls back to 1 instead of 0.
