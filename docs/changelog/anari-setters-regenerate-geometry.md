## ANARI triangle and glyph options apply to existing geometry

`ANARIMapperTriangles::SetCalculateNormals` and
`ANARIMapperGlyphs::SetOffsetGlyphs` used to only store the new value. Once
the mapper had built its geometry, the change was ignored until the next
`SetActor`. Changing either option now rebuilds the geometry's arrays when
the geometry already exists. Turning normals off removes `vertex.normal` and
releases the normals array, which used to stay in memory.
