## ANARI mappers warn about fields they skip, and the volume mapper checks its input

The ANARI mappers now handle a field they cannot use in one way. A field that
only adds color or shading is skipped with a warning naming the field and the
reason, and the rest of the actor is still drawn. `ANARIMapperTriangles` used
to skip such fields silently: a field with more than one component, one that
is not a point or cell field, or one with the wrong number of values. It also
silently dropped calculated normals that did not have one value per point. It
now logs a warning for each, as `ANARIMapperPoints` already did. A field the
mapper cannot draw without, the scalar field of `ANARIMapperVolume` or the
vector field of `ANARIMapperGlyphs`, still makes the mapper log a warning and
leave its group empty.

`ANARIMapperVolume` now also rejects its actor when the field does not have
one value per point (point field) or per cell (cell field), when the field is
not a point or cell field, and when the connectivity of an unstructured cell
set contains a negative point index, which used to wrap to a large unsigned
index. The triangles, points and glyphs mappers now reject an actor with
more primitives than their `NumPrimitives` count can hold, instead of
truncating the count.
