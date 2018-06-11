# Support initializer lists for Vec

Add constructors to the `vtkm::Vec` classes that accept `std::initializer_list`. The
main advantage of this addition is that it makes it much easier to initialize `Vec`s
of arbitrary length.

Although previously some `Vec` classes could be constructed with values listed in
their parameters, that only worked for initializes up to size 4.

``` cpp
vtkm::Vec<vtkm::Float64, 3> vec1{1.1, 2.2, 3.3};  // New better initializer

vtkm::Vec<vtkm::Float64, 3> vec2 = {1.1, 2.2, 3.3}; // Nice syntax also supported by
                                                    // initializer lists.

vtkm::Vec<vtkm::Float64, 3> vec3(1.1, 2.2, 3.3); // Old style that still works but
                                                 // probably should be deprecated.
```

Nested initializer lists work to initialize `Vec`s of `Vec`s.

``` cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec{ {1.1, 2.2}, {3.3, 4.4}, {5.5, 6.6} };
```

The nice thing about the `std::initializer_list` implementation is that it works for any
size `Vec`. That keeps us from jumping through hoops for larger `Vec`s.

``` cpp
vtkm::Vec<vtkm::Float64, 5> vec1{1.1, 2.2, 3.3, 4.4, 5.5}; // Works fine.

vtkm::Vec<vtkm::Float64, 5> vec2(1.1, 2.2, 3.3, 4.4, 5.5); // ERROR! This constructor
                                                           // not implemented!
```

If a `vtkm::Vec` is initialized with a list of size one, then that one value is
replicated for all components.

``` cpp
vtkm::Vec<vtkm::Float64, 3> vec{1.1};  // vec gets [ 1.1, 1.1, 1.1 ]
```

This "scalar" initialization also works for `Vec`s of `Vec`s.

``` cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec1{ { 1.1, 2.2 } };
// vec1 is [[1.1, 2.2], [1.1, 2.2], [1.1, 2.2]]

vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec2{ { 3.3}, { 4.4 }, { 5.5 } };
// vec2 is [[3.3, 3.3], [4.4, 4.4], [5.5, 5.5]]
```

`vtkm::make_Vec` is also updated to support an arbitrary number initial values.

``` cpp
// Creates a vtkm::Vec<vtkm::Float64, 5>
auto vec = vtkm::make_Vec(1.1, 2.2, 3.3, 4.4, 5.5);
```

This is super convenient when dealing with variadic function arguments.

``` cpp
template <typename... Ts>
void ExampleVariadicFunction(const Ts&... params)
{
  auto vec = vtkm::make_Vec(params...);
```

Of course, this assumes that the type of all the parameters is the same. If not, you
could run into compiler trouble.

There is also a version of `vtkm::make_Vec` that accepts an `std::initializer_list`,
but it is not very useful because you have to separately specify the length of the
`Vec` (due to some limitations of `std::initializer_list` in C++11).

``` cpp
// Creates a vtkm::Vec<vtkm::Float64, 3>
auto vec1 = vtkm::make_Vec<3>({1.1, 2.2, 3.3});

// Creates exactly the same thing
auto vec2 = vtkm::Vec<vtkm::Float64, 3>{1.1, 2.2, 3.3};
```

A limitation of the initializer list constructor is that the compiler has no way to
check the length of the list or force it to a particular length. Thus it is entirely
possible to construct a `Vec` with the wrong number of arguments. Or, more to the
point, the compiler will let you do it, but there is an assert in the constructor to
correct for that. (Of course, asserts are not compiled in release builds.)

``` cpp
// This will compile, but it's results are undefined when it is run.
// In debug builds, it will fail an assert.
vtkm::Vec<vtkm::Float64, 3> vec{1.1, 1.2};
```
