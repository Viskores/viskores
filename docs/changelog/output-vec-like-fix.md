# Support using arrays with dynamic Vec-likes as output arrays

When you use an `ArrayHandle` as an output array in a worklet (for example,
as a `FieldOut`), the fetch operation does not read values from the array
during the `Load`. Instead, it just constructs a new object. This makes
sense as an output array is expected to have garbage in it anyway.

This is a problem for some special arrays that contain `Vec`-like objects
that are sized dynamically. For example, if you use an
`ArrayHandleGroupVecVariable`, each entry is a dynamically sized `Vec`. The
array is referenced by creating a special version of `Vec` that holds a
reference to the array portal and an index. Components are retrieved and
set by accessing the memory in the array portal. This allows us to have a
dynamically sized `Vec` in the execution environment without having to
allocate within the worklet.

The problem comes when we want to use one of these arrays with `Vec`-like
objects for an output. The typical fetch fails because you cannot construct
one of these `Vec`-like objects without an array portal to bind it to. In
these cases, we need the fetch to create the `Vec`-like object by reading
it from the array. Even though the data will be garbage, you get the
necessary buffer into the array (and nothing more).

Previously, the problem was fixed by creating partial specializations of
the `Fetch` for these `ArrayHandle`s. This worked OK as long as you were
using the array directly. However, the approach failed if the `ArrayHandle`
was wrapped in another `ArrayHandle` (for example, if an `ArrayHandleView`
was applied to an `ArrayHandleGroupVecVariable`).

To get around this problem and simplify things, the basic `Fetch` for
direct output arrays is changed to handle all cases where the values in the
`ArrayHandle` cannot be directly constructed. A compile-time check of the
array's value type is checked with `std::is_default_constructible`. If it
can be constructed, then the array is not accessed. If it cannot be
constructed, then it grabs a value out of the array.

