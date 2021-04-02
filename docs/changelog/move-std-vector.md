# Improvements to moving data into ArrayHandle

We have made several improvements to adding data into an `ArrayHandle`.

## Moving data from an `std::vector`

For numerous reasons, it is convenient to define data in a `std::vector`
and then wrap that into an `ArrayHandle`. There are two obvious ways to do
this. First, you could deep copy the data into an `ArrayHandle`, which has
obvious drawbacks. Second, you could take the pointer for the data in the
`std::vector` and use that as user-allocated memory in the `ArrayHandle`
without deep copying it. The problem with this shallow copy is that it is
unsafe. If the `std::vector` goes out of scope (or gets resized), then the
data the `ArrayHandle` is pointing to becomes unallocated, which will lead
to unpredictable behavior.

However, there is a third option. It is often the case that an
`std::vector` is filled and then becomes unused once it is converted to an
`ArrayHandle`. In this case, what we really want is to pass the data off to
the `ArrayHandle` so that the `ArrayHandle` is now managing the data and
not the `std::vector`.

C++11 has a mechanism to do this: move semantics. You can now pass
variables to functions as an "rvalue" (right-hand value). When something is
passed as an rvalue, it can pull state out of that variable and move it
somewhere else. `std::vector` implements this movement so that an rvalue
can be moved to another `std::vector` without actually copying the data.
`make_ArrayHandle` now also takes advantage of this feature to move rvalue
`std::vector`s.

There is a special form of `make_ArrayHandle` named `make_ArrayHandleMove`
that takes an rvalue. There is also a special overload of
`make_ArrayHandle` itself that handles an rvalue `vector`. (However, using
the explicit move version is better if you want to make sure the data is
actually moved.)

So if you create the `std::vector` in the call to `make_ArrayHandle`, then
the data only gets created once.

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>{ 2, 6, 1, 7, 4, 3, 9 });
```

Note that there is now a better way to express an initializer list to
`ArrayHandle` documented below. But this form of `ArrayHandleMove` can be
particularly useful for initializing an array to all of a particular value.
For example, an easy way to initialize an array of 1000 elements all to 1
is

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>(1000, 1));
```

You can also move the data from an already created `std::vector` by using
the `std::move` function to convert it to an rvalue. When you do this, the
`std::vector` becomes invalid after the call and any use will be undefined.

``` cpp
std::vector<vtkm::Id> vector;
// fill vector

auto array = vtkm::cont::make_ArrayHandleMove(std::move(vector));
```

## Make `ArrayHandle` from initalizer list

A common use case for using `std::vector` (particularly in our unit tests)
is to quickly add an initalizer list into an `ArrayHandle`. Repeating the
example from above:

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>{ 2, 6, 1, 7, 4, 3, 9 });
```

However, creating the `std::vector` should be unnecessary. Why not be able
to create the `ArrayHandle` directly from an initializer list? Now you can
by simply passing an initializer list to `make_ArrayHandle`.

``` cpp
auto array = vtkm::cont::make_ArrayHandle({ 2, 6, 1, 7, 4, 3, 9 });
```

There is an issue here. The type here can be a little ambiguous (for
humans). In this case, `array` will be of type
`vtkm::cont::ArrayHandleBasic<int>`, since that is what an integer literal
defaults to. This could be a problem if, for example, you want to use
`array` as an array of `vtkm::Id`, which could be of type `vtkm::Int64`.
This is easily remedied by specifying the desired value type as a template
argument to `make_ArrayHandle`.

``` cpp
auto array = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 2, 6, 1, 7, 4, 3, 9 });
```

## Deprecated `make_ArrayHandle` with default shallow copy

For historical reasons, passing an `std::vector` or a pointer to
`make_ArrayHandle` does a shallow copy (i.e. `CopyFlag` defaults to `Off`).
Although more efficient, this mode is inherintly unsafe, and making it the
default is asking for trouble.

To combat this, calling `make_ArrayHandle` without a copy flag is
deprecated. In this way, if you wish to do the faster but more unsafe
creation of an `ArrayHandle` you should explicitly express that.

This requried quite a few changes through the VTK-m source (particularly in
the tests).

## Similar changes to `Field`

`vtkm::cont::Field` has a `make_Field` helper function that is similar to
`make_ArrayHandle`. It also features the ability to create fields from
`std::vector`s and C arrays. It also likewise had the same unsafe behavior
by default of not copying from the source of the arrays.

That behavior has similarly been depreciated. You now have to specify a
copy flag.

The ability to construct a `Field` from an initializer list of values has
also been added.
