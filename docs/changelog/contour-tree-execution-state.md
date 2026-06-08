## Contour Tree Filter No Longer Stores Execution Scratch as Filter State

The `ContourTreeUniformDistributed` filter previously held its intermediate
results (local meshes, contour trees, boundary trees, interior forests, and the
iteration count) as filter member state. Because filters can be copied, copies
could share or clobber this mutable scratch when executed independently.

These intermediate results are now collected in a local `ExecutionState` object
that lives only for the duration of a single execution and is passed by
reference through the filter's helper functions. Copies of the filter no longer
share execution scratch.
