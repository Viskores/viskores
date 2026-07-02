## Give ANARI mappers coherent ownership

ANARI mappers are now move-only and uniquely own their ANARI handles, backing
Viskores arrays, actor, options, and update state. `ANARIScene::AddMapper` and
`ANARIScene::ReplaceMapper` consume mappers instead of copying them, and reject
mappers created for a different ANARI device.

`ANARIActor` now has value semantics, so changing the primary field on one
actor copy does not silently change a materialized mapper.
