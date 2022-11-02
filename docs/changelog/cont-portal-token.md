# Support providing a Token to ReadPortal and WritePortal

When managing portals in the execution environment, `ArrayHandle` uses the
`Token` object to ensure that the memory associated with a portal exists
for the length of time that it is needed. This is done by creating the
portal with a `Token` object, and the associated portal objects are
guaranteed to be valid while that `Token` object exists. This is supported
by essentially locking the array from further changes.

`Token` objects are typically used when creating a control-side portal with
the `ReadPortal` or `WritePortal`. This is not to say that a `Token` would
not be useful; a control-side portal going out of scope is definitely a
problem. But the creation and distruction of portals in the control
environment is generally too much work for the possible benefits.

However, under certain circumstances it could be useful to use a `Token` to
get a control-side portal. For example, if the `PrepareForExecution` method
of an `ExecutionObjectBase` needs to fill a small `ArrayHandle` on the
control side to pass to the execution side, it would be better to use the
provided `Token` object when doing so. This change allows you to optionally
provide that `Token` when creating these control-side portals.
