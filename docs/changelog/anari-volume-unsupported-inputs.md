## Reject unsupported inputs in the ANARI volume mapper

`ANARIMapperVolume` sent any actor it could not map as a structured scalar volume
down the unstructured path, even when that path could not handle it. An empty
cell set, a non-scalar field, or a cell set other than `CellSetSingleType<>` or
`CellSetExplicit<>` then crashed or threw while creating ANARI arrays. The mapper
now logs a warning for these actors and maps them to an empty group with no
spatial field.
