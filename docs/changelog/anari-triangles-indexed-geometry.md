## ANARI triangles mapper sends indexed geometry and maps cell fields

`ANARIMapperTriangles` read every field by point ID, so a cell field
colored the surface with the wrong values or read past the end of the
field. Cell fields now become `primitive.attributeN` arrays with one value
per triangle, taken from the triangle's cell. Point fields stay
`vertex.attributeN` arrays.

The mapper also stopped building a separate copy of every vertex, normal
and field value for each triangle. It now shares the coordinates, point
normals and point fields with ANARI, copying them only when they must be
converted to 32-bit floats, and sends the triangles as a `primitive.index`
array. Meshes with more points than 32-bit indices can address get
`ANARI_UINT64_VEC3` indices and a warning. For a 10-million-triangle
surface, the memory the mapper keeps drops from about 590 MB to about
120 MB, and the peak while building it from about 890 MB to about 630 MB.

`TrianglesParameters` gains `Primitive.Attribute`, and the field names
move from `Vertex.AttributeName` to `AttributeName`, since a slot can hold
either a vertex or a primitive attribute.
