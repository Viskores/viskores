## Route flow seeds with BoundsMap locator worklets

Flow particle seeds are now mapped to candidate blocks with the BoundsMap
cell-id locator from worklets. Candidate ordering, first-block selection, rank
ownership filtering, and compaction remain in ArrayHandles until the existing
host queue handoff. Locator results are checked against the original Float64
block bounds on the device so coordinate narrowing cannot change seed
ownership.
