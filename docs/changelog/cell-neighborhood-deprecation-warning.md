## Fix deprecation warning in WorkletCellNeighborhood

There was a use of a deprecated method buried in a support class of
`WorkletCellNeighborhood`. This fixes that deprecation and also adds a
missing test for `WorkletCellNeighborhood` to prevent such things in the
future.
