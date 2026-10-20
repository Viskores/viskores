## ANARI glyphs handle zero vectors, unsupported fields and single points

`ANARIMapperGlyphs` used to produce NaN cone vertices for a zero-length
vector, which can corrupt or crash some ANARI devices. Such vectors (and
non-finite ones) now get a zero-size glyph at their point. A field that does
not have 3 components, or is not a point or cell field of matching size, used
to throw from `GetANARISurface()`; the mapper now logs a warning and leaves
its group empty. When the coordinate bounds have zero size, such as a single
point, the glyph size falls back to 1 instead of 0.
