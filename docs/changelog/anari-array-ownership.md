## ANARI arrays now own the Viskores memory they share

The ANARI mappers shared Viskores host memory with ANARI arrays through a
no-op deleter, so the memory lived only as long as the mapper that made it.
An application that retained a mapper's geometry or spatial field, and then
destroyed the mapper, left the device reading freed memory. The mappers now
create their arrays with `MakeANARIArray1D` and `MakeANARIArray3D` (in
`viskores/interop/anari/ANARIMapper.h`). These functions hand the device a
reference to the Viskores array, which it drops when it destroys the ANARI
array, so retained handles stay valid after the mapper is gone. While the
device holds such an array, writes to the Viskores array it shares wait, so
release retained handles before you modify a field that a mapper shares
without copying. A write on the thread that still holds them deadlocks. The
headers now document this, and that copies of a mapper (including the copy
`ANARIScene::AddMapper` stores) share the same ANARI objects.

We removed the `TriangleArrays`, `TriangleFieldArrays`, `PointsArrays`,
`PointsFieldArrays`, `GlyphArrays`, `StructuredVolumeArrays` and
`UntructuredVolumeArrays` structs from the public headers. They only described
mapper internals.
