# Refactor topology mappings to clarify meaning.

The `From` and `To` nomenclature for topology mapping has been confusing for
both users and developers, especially at lower levels where the intention of
mapping attributes from one element to another is easily conflated with the
concept of mapping indices (which maps in the exact opposite direction).

These identifiers have been renamed to `VisitTopology` and `IncidentTopology`
to clarify the direction of the mapping. The order in which these template
parameters are specified for `WorkletMapTopology` have also been reversed,
since eventually there may be more than one `IncidentTopology`, and having
`IncidentTopology` at the end will allow us to replace it with a variadic
template parameter pack in the future.

Other implementation details supporting these worklets, include `Fetch` tags,
`Connectivity` classes, and methods on the various `CellSet` classes (such as
`PrepareForInput` have also reversed their template arguments. These will need
to be cautiously updated.

The convenience implementations of `WorkletMapTopology` have been renamed for
clarity as follows:

```
WorkletMapPointToCell --> WorkletVisitCellsWithPoints
WorkletMapCellToPoint --> WorkletVisitPointsWithCells
```

The `ControlSignature` tags have been renamed as follows:

```
FieldInTo --> FieldInVisit
FieldInFrom --> FieldInMap
FromCount --> IncidentElementCount
FromIndices --> IncidentElementIndices
```
