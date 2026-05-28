## Fixed error about ambiguous thrust namespace

There are a few places in Viskores that have to modify the internal behavior of
Thrust to, for example, properly identify a reference object as a read/write
reference. However, recent versions of thrust sometimes caused a compiler error
about the `thrust::detail` namespace being ambiguous. This is because Thrust has
its special ABI that declares things in anonymous spaces to avoid conflicts.

When Viskores touches internal components of Thrust, it now uses the
`THRUST_NAMESPACE_BEGIN`/`END` macros to define the namespace so it follows how
things are defined by Thrust.
