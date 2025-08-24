## Enable more types for Clamp function

The Viskores `Clamp` function now works with any numeric type. Previously it
only worked with basic floating point types (i.e., `viskores::Float32` and
`viskores::Float64`). It is now templated to work with any numeric type. It can
now also operate on `Vec` and `Vec`-like types. Furthermore, it is possible to
mix the types of the arguments without ambiguous overloading.

Documentation for `Clamp` is now in the user's guide whereas previously it was
missing.
