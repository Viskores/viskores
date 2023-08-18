# Get the 3D index from a BoundaryState in WorkletPointNeighborhood

There are occasions when you need a worklet to opeate on 2D or 3D indices.
Most worklets operate on 1D indices, which requires recomputing the 3D
index in each worklet instance. A workaround is to use a worklet that does
a 3D scheduling and pull the working index from that.

The problem was that there was no easy way to get this 3D index. To provide
this option, a feature was added to the `BoundaryState` class that can be
provided by `WorkletPointNeighborhood`.

Thus, to get a 3D index in a worklet, use the `WorkletPointNeighborhood`,
add `Boundary` as an argument to the `ExecutionSignature`, and then call
`GetCenterIndex` on the `BoundaryState` object passed to the worklet
operator.
