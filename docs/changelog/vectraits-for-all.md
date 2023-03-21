# Implemented `VecTraits` class for all types

The `VecTraits` class allows templated functions, methods, and classes to
treat type arguments uniformly as `Vec` types or to otherwise differentiate
between scalar and vector types. This only works for types that `VecTraits`
is defined for.

The `VecTraits` templated class now has a default implementation that will
be used for any type that does not have a `VecTraits` specialization. This
removes many surprise compiler errors when using a template that, unknown
to you, has `VecTraits` in its implementation.

One potential issue is that if `VecTraits` gets defined for a new type, the
behavior of `VecTraits` could change for that type in backward-incompatible
ways. If `VecTraits` is used in a purely generic way, this should not be an
issue. However, if assumptions were made about the components and length,
this could cause problems.

Fixes #589.
