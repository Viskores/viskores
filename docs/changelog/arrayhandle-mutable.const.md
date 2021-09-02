# Allow a `const ArrayHandle` to be reallocated

Previously, the `Allocate` method of `ArrayHandle` was _not_ declared as
`const`. Likewise, the methods that depended on `Allocate`, namely
`ReleaseResources` and `PrepareForOutput` were also not declared `const`.
The main consequence of this was that if an `ArrayHandle` were passed as a
constant reference argument to a method (e.g. `const ArrayHandle<T>& arg`),
then the array could not be reallocated.

This seems right at first blush. However, we have changed these methods to
be `const` so that you can in fact reallocate the `ArrayHandle`. This is
because the `ArrayHandle` is in principle a pointer to an array pointer.
Such a structure in C will allow you to change the pointer to the array,
and so in this context it makes sense for `ArrayHandle` to support that as
well.

Although this distinction will certainly be confusing to users, we think
this change is correct for a variety of reasons.

  1. This change makes the behavior of `ArrayHandle` consistent with the
     behavior of `UnknownArrayHandle`. The latter needed this behavior to
     allow `ArrayHandle`s to be passed as output arguments to methods that
     get automatically converted to `UnknownArrayHandle`.
  2. Before this change, a `const ArrayHandle&` was still multible is many
     way. In particular, it was possible to change the data in the array
     even if the array could not be resized. You could still call things
     like `WritePortal` and `PrepareForInOut`. The fact that you could
     change it for some things and not others was confusing. The fact that
     you could call `PrepareForInOut` but not `PrepareForOutput` was doubly
     confusing.
  3. Passing a value by constant reference should be the same, from the
     calling code's perspective, as passing by value. Although the function
     can change an argument passed by value, that change is not propogated
     back to the calling code. However, in the case of `ArrayHandle`,
     calling by value would allow the array to be reallocated from the
     calling side whereas a constant reference would prevent that. This
     change makes the two behaviors consistent.
  4. The supposed assurance that the `ArrayHandle` would not be reallocated
     was easy to break even accidentally. If the `ArrayHandle` was assigned
     to another `ArrayHandle` (for example as a class' member or wrapped
     inside of an `UnknownArrayHandle`), then the array was free to be
     reallocated.
