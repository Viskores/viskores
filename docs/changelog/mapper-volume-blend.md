## Fixed bug with `MapperVolume` blending

When writing cast rays back to a frame buffer, the canvas records both the color
and the depth. The depth is computed from where the ray intersected the data.
However, `MapperVolume` does not record this intersection depth because it
traces through the whole transparent volume. This was causing bogus depth to be
written to the buffer, and that was interfering when rendering multiple
partitions.

This problem is fixed by adding an option to the `Canvas::WriteToCanvas` method
that prevents the depth buffer from being updated. The `MapperVolume` uses this
option to prevent writing bad depth values.
