# Fixed issue with trivial variant copies

A rare error occurred with trivial copies of variants. The problem is likely
a compiler bug, and has so far only been observed when passing the variant
to a CUDA kernel when compiling with GCC 5.

The problem was caused by structures with padding. `struct` objects in
C/C++ are frequently padded with unused memory to align all entries
properly. For example, consider the following simple `struct`.

``` cpp
struct FooHasPadding
{
  vtkm::Int32 A;
  // Padding here.
  vtkm::Int64 C;
};
```

Because the `C` member is a 64-bit integer, it needs to be aligned on
8-byte (i.e., 64-bit) address locations. For this to work, the C++ compiler
adds 4 bytes of padding between `A` and `C` so that an array of
`FooHasPadding`s will have the `C` member always on an 8-byte boundary.

Now consider a second `struct` that is similar to the first but has a valid
member where the padding would be.

``` cpp
struct BarNoPadding
{
  vtkm::Int32 A;
  vtkm::Int32 B;
  vtkm::Int64 C;
};
```

This structure does not need padding because the `A` and `B` members
combine to fill the 8 bytes that `C` needs for the alignment. Both
`FooHasPadding` and `BarNoPadding` fill 16 bytes of memory. The `A` and `C`
members are at the same offsets, respectively, for the two structures. The
`B` member happens to reside just where the padding is for `FooHasPadding`.

Now, let's say we create a `vtkm::exec::Variant<FooHasPadding, BarNoPadding>`.
Internally, the `Variant` class holds a union that looks roughly like the
following.

``` cpp
union VariantUnion
{
  FooHasPadding V0;
  BarNoPadding V1;
};
```

This is a perfectly valid use of a `union`. We just need to keep track of
which type of object is in it (which the `Variant` object does for you).

The problem appeared to occur when `VariantUnion` contained a
`BarNoPadding` and was passed from the host to the device via an argument
to a global function. The compiler must notice that the first type
(`FooHasPadding`) is the "biggest" and uses that for trivial copies (which
just copy bytes like `memcpy`). Since it's using `FooHasPadding` as its
prototype for the byte copy, and accidentally skips over padded regions that
are valid when the `union` contains a `BarNoPadding`. This appears to be a
compiler bug. (At least, I cannot find a reason why this is encroaching
undefined behavior.)

The solution adds a new, unused type to the internal `union` for `Variant`
that is an object as large as the largest entry in the union and contains
no padding.
