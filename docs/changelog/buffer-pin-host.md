## Add Buffer::PinHost for stable host-pointer access

`viskores::cont::internal::Buffer` now provides a `PinHost(Token&)` method
that locks the host allocation in place for the remainder of the `Buffer`
object's lifetime. The host pointer is guaranteed not to move after PinHost
is called; any subsequent attempt to grow the buffer raises
`viskores::cont::ErrorBadAllocation` rather than silently relocating the
allocation. The operation is intentionally irreversible — callers that need
only bounded-lifetime pointer stability should hold a
`viskores::cont::Token` instead.

This is intended for handing a buffer's host pointer to external libraries
that manage memory independently. For example, the Python bindings use it
to wrap a Viskores-allocated `ArrayHandleBasic` in a zero-copy NumPy view
whose lifetime is decoupled from the originating `ArrayHandle`.
