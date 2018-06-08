# New geometry classes and header.

There are now some additional structures available both
the control and execution environments for representing
geometric entities (mostly of dimensions 2 and 3).
These new structures are now in `vtkm/Geometry.h` and
demonstrated/tested in `vtkm/testing/TestingGeometry.h`:

+ `Ray<CoordType, Dimension, IsTwoSided>`.
  Instances of this struct represent a semi-infinite line
  segment in a 2-D plane or in a 3-D space, depending on the
  integer dimension specified as a template parameter.
  Its state is the point at the start of the ray (`Origin`)
  plus the ray's `Direction`, a unit-length vector.
  If the third template parameter (IsTwoSided) is true, then
  the ray serves as an infinite line. Otherwise, the ray will


+ `LineSegment<CoordType, Dimension>`.
  Instances of this struct represent a finite line segment
  in a 2-D plane or in a 3-D space, depending on the integer
  dimension specified as a template parameter.
  Its state is the coordinates of its `Endpoints`.
+ `Plane<CoordType>`.
  Instances of this struct represent a plane in 3-D.
  Its state is the coordinates of a base point (`Origin`) and
  a unit-length normal vector (`Normal`).
+ `Sphere<CoordType, Dimension>`.
  Instances of this struct represent a *d*-dimensional sphere.
  Its state is the coordinates of its center plus a radius.
  It is also aliased with a `using` statment to `Circle<CoordType>`
  for the specific case of 2-D.

These structures provide useful queries and generally
interact with one another.
For instance, it is possible to intersect lines and planes
and compute distances.

For ease of use, there are also several `using` statements
that alias these geometric structures to names that specialize
them for a particular dimension or other template parameter.
As an example, `Ray<CoordType, Dimension, true>` is aliased
to `Line<CoordType, Dimension>` and `Ray<CoordType, 3, true>`
is aliased to `Line3<CoordType>` and `Ray<FloatDefault, 3, true>`
is aliased to `Line3d`.

## Design patterns

If you plan to add a new geometric entity type,
please adopt these conventions:

+ Each geometric entity may be default-constructed.
  The default constructor will initialize the state to some
  valid unit-length entity, usually with some part of
  its state at the origin of the coordinate system.
+ Entities may always be constructed by passing in values
  for their internal state.
  Alternate construction methods are declared as free functions
  such as `make_CircleFrom3Points()`
+ Use template metaprogramming to make methods available
  only when the template dimension gives them semantic meaning.
  For example, a 2-D line segment's perpendicular bisector
  is another line segment, but a 3-D line segment's perpendicular
  line segment is a plane.
  Note how this is accomplished and apply this pattern to
  new geometric entities or new methods on existing entities.
+ Some entities may have invalid state.
  If this is possible, the entity will have an `IsValid()` method.
  For example, a sphere may be invalid because the user or some
  construction technique specified a zero or negative radius.
+ When signed distance is semantically meaningful, provide it
  in favor of or in addition to unsigned distance.
+ Accept a tolerance parameter when appropriate,
  but provide a sensible default value.
  You may want to perform exact arithmetic versions of tests,
  but please provide fast, tolerance-based versions as well.
