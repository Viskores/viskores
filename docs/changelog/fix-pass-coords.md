# Respect `Filter::PassCoordinateSystem` flag in filters creating coordinate systems

The `Filter` class has a `PassCoordinateSystem` flag that specifies whether
coordinate systems should be passed regardless of whether the associated
field is passed. However, if a filter created its output with the
`CreateResultCoordinateSystem` method this flag was ignored, and the
provided coordinate system was always passed. This might not be what the
user intended, so this method has been fixed to first check the
`PassCoordinateSystem` flag before setting the coordinates on the output.
