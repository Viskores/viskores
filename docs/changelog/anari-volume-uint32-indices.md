## Send 32-bit indices from the ANARI unstructured volume mapper

`ANARIMapperVolume` sent the `index` and `cell.index` arrays of an unstructured
spatial field as 64-bit integers, but the ANARI `KHR_SPATIAL_FIELD_UNSTRUCTURED`
extension requires `ANARI_UINT32`. Devices that check the element type could
reject or misread them. The mapper now converts connectivity and offsets to
32-bit integers, which also halves the memory they use. Cell sets whose
connectivity does not fit in 32-bit indices are rejected with a warning and
mapped to an empty group.
