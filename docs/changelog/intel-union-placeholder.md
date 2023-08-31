# Fix issue with union placeholder on Intel compiler

We have run into an issue with some Intel compilers where if a `union`
contains a `struct` that has some padding for byte alignment, the value
copy might skip over that padding even when the `union` contains a different
type where those bytes are valid. This breaks the value copy of our
`Variant` class.

This is not a unique problem. We have seen the same thing in other
compilers and already have a workaround for when this happens. The
workaround creates a special struct that has no padding placed at the front
of the `union`. The Intel compiler adds a fun twist in that this
placeholder structure only works if the alignment is at least as high as
the struct that follows it.

To get around this problem, make the alignment of the placeholder `struct`
at large as possible for the size of the `union`.
