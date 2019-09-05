# Fix cell derivatives for polygon cell shape

For polygon cell shapes (that are not triangles or quadrilaterals),
interpolations are done by finding the center point and creating a triangle
fan around that point. Previously, the gradient was computed in the same
way as interpolation: identifying the correct triangle and computing the
gradient for that triangle.

The problem with that approach is that makes the gradient discontinuous at
the boundaries of this implicit triangle fan. To make things worse, this
discontinuity happens right at each vertex where gradient calculations
happen frequently. This means that when you ask for the gradient at the
vertex, you might get wildly different answers based on floating point
imprecision.

Get around this problem by creating a small triangle around the point in
question, interpolating values to that triangle, and use that for the
gradient. This makes for a smoother gradient transition around these
internal boundaries.
