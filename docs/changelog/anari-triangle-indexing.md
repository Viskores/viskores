## Preserve indexed ANARI triangles and field association

`ANARIMapperTriangles` now keeps one position per source point and emits the
triangle extractor's connectivity as an explicit `primitive.index` array.
Point-associated scalar fields become vertex attributes, while cell-associated
scalar fields become primitive attributes selected with the extractor's source
cell ID. Whole-dataset and nonscalar fields are ignored.

For a representative 257 by 257 point grid (65,536 quads and 131,072
triangles), the old and new array item counts are:

| Array | Expanded | Indexed |
| --- | ---: | ---: |
| `vertex.position` | 393,216 | 66,049 |
| point `vertex.attribute0` | 393,216 | 66,049 |
| `vertex.normal` when enabled | 393,216 | 66,049 |
| `primitive.index` triples | 131,072 | 131,072 |

The indexed representation reduces each point-associated array by 83 percent
on this mesh. A data-level regression test records these position, point-field,
and index counts independently of image comparison.
