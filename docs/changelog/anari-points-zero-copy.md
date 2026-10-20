## ANARI points mapper shares coordinates and checks fields

`ANARIMapperPoints` no longer runs `SphereExtractor`, which built an index
array and a radius array for every point before the mapper copied each
coordinate through that index. The mapper now shares the coordinates with
ANARI as `vertex.position`, copying them only when they must be converted
to 32-bit floats, and sets one radius for all spheres through the sphere
geometry's `radius` parameter. For 20 million points, the memory the mapper
keeps drops from about 307 MB to about 2 MB, and the peak while building
the geometry from about 450 MB to about 2 MB.

Only fields with one value per point become `vertex.attributeN` arrays.
Other fields, such as a cell field on a data set with a different number of
cells, are skipped with a warning; before, they were attached anyway. Every
attached attribute now gets its USD name, not only 1-component ones. A
data set whose points all lie in one place, such as a single point, now gets
a sphere radius of 1 instead of 0.

`PointsParameters::Vertex.Radius` is replaced by `PointsParameters::Radius`,
a single value.
