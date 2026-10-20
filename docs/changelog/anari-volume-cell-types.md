## Fix hexahedron and pyramid cell types in the ANARI volume mapper

`ANARIMapperVolume` sent hexahedra to ANARI as pyramids and pyramids as hexahedra,
so unstructured hexahedral volumes rendered incorrectly. The mapper now sends the
`cell.type` values the ANARI spec defines: 10 for tetrahedra, 12 for hexahedra,
13 for wedges and 14 for pyramids.
