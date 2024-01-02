# Allow floating-point isovalues for contours of integer fields

The flying edges version of the contouring filter converted the isovalues
provided into the same type as the field. This is fine for a floating point
field, but for an integer field the isovalue was truncated to the nearest
integer.

This is problematic because it is common to provide a fractional isovalue
(usually N + 0.5) for integer fields to avoid degenerate cases of the
contour intersecting vertices. It also means the behavior changes between
an integer type that is directly supported (like a `signed char`) or an
integer type that is not directly supported and converted to a floating
point field (like potentially a `char`).

This change updates the worklets to allow the isovalue to have a different
type than the field and to always use a floating point type for the
isovalue.
