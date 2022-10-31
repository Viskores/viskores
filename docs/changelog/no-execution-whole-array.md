# Removed ExecutionWholeArray class

`ExecutionWholeArray` is an archaic class in VTK-m that is a thin wrapper
around an array portal. In the early days of VTK-m, this class was used to
transfer whole arrays to the execution environment. However, now the
supported method is to use `WholeArray*` tags in the `ControlSignature` of
a worklet.

Nevertheless, the `WholeArray*` tags caused the array portal transferred to
the worklet to be wrapped inside of an `ExecutionWholeArray` class. This
is unnecessary and can cause confusion about the types of data being used.

Most code is unaffected by this change. Some code that had to work around
the issue of the portal wrapped in another class used the `GetPortal`
method which is no longer needed (for obvious reasons). One extra feature
that `ExecutionWholeArray` had was that it provided an subscript operator
(somewhat incorrectly). Thus, any use of '[..]' to index the array portal
have to be changed to use the `Get` method.
