## Complete ANARI unstructured-volume lowering

`ANARIMapperVolume` now lowers `CellSetSingleType` and `CellSetExplicit` volumes
to the current `ANARI_KHR_SPATIAL_FIELD_UNSTRUCTURED` contract. Connectivity and
cell offsets are validated before conversion to `UInt32`, supported Viskores/VTK
cell IDs are preserved, and point- and cell-associated scalar data are set on
`vertex.data` and `cell.data`, respectively.

Unsupported cell sets, cell shapes, malformed topology, oversized indices, and
incorrect field cardinality are rejected before ANARI objects are created.
Spatial-field updates construct a complete candidate before replacing the prior
representation, and all backing ANARI arrays are released with that state.
