# Update filters' field map and execute to work on any field type

Several filters implemented their map field by checking for common field
types and interpolated those. Although there was a float fallback to catch
odd component types, there were still a couple of issues. First, it meant
that several types got converted to `vtkm::FloatDefault`, which is often at
odds with how VTK handles it. Second, it does not handle all `Vec` lengths,
so it is still possible to drop fields.

The map field functions for these filters have been changed to support all
possible types. This is done by using the extract component functionality
to get data from any type of array. The following filters have been
updated. In some circumstances where it makes sense, a simple float
fallback is used.

  * `CleanGrid`
  * `CellAverage`
  * `ClipWithField`
  * `ClipWithImplicitFunction`
  * `Contour`
  * `MIRFilter`
  * `NDHistogram`
  * `ParticleDensityCloudInCell`
  * `ParticleDensityNearestGridPoint`
  * `PointAverage`
  * `Probe`
  * `VectorMagnitude`
