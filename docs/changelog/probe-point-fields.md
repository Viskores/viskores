# Probe always generates point fields

Previously, the `probe` filter, when probing the input's cell fields, would
store the result as the output's cell field. This is a bug since the probing is
done at the geometry's point locations, and the output gets its structure from
the `geometry`.

This behaviour is fixed in this release. Now, irrespective of the type of the
input field being probed, the result field is always a point field.
