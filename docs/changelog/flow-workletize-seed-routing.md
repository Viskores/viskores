## Route flow seeds with BoundsMap locator worklets

Flow particle seeds are now mapped to candidate blocks with the BoundsMap
cell-id locator from a worklet. This removes the host-side bounds scan from
initial seed routing while preserving the existing rank ownership behavior.
