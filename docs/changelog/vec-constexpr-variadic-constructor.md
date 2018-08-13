# Support constexpr and variadic constructor for Vec

Add variadic constructors to the `vtkm::Vec` classes. The main advantage of this addition
is that it makes it much easier to initialize `Vec`s of arbitrary length.

Meanwhile, `Vec` classes constructed with values listed in their parameters up to size 4
are constructed as constant expressions at compile time to reduce runtime overhead.
Sizes greater than 4 are not yet supported to be constructed at compile time via initializer lists
since in C++11 constexpr does not allow for loops. Only on Windows platform with a compiler older
than Visual Studio 2017 version 15.0, users are allowed to use initializer lists to construct a
vec with size > 4.

`vtkm::make_Vec` would always construct `Vec` at compile time if possible.

``` cpp
vtkm::Vec<vtkm::Float64, 3> vec1{1.1, 2.2, 3.3};  // New better initializer since
                                                  // it does not allow type narrowing

vtkm::Vec<vtkm::Float64, 3> vec2 = {1.1, 2.2, 3.3}; // Nice syntax also supported by
                                                    // initializer lists.

vtkm::Vec<vtkm::Float64, 3> vec3 = vtkm::make_Vec(1.1, 2.2, 3.3); // Old style that still works.

vtkm::Vec<vtkm::Float64, 3> vec3(1.1, 2.2, 3.3); // Old style that still works but
                                                 // should be deprecated. Reason listed below.
```

Nested initializer lists work to initialize `Vec` of `Vec`s. If the size is no more than 4,
it's always constructed at compile time if possible.

``` cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec{ {1.1, 2.2}, {3.3, 4.4}, {5.5, 6.6} };
                                                           //Constructed at compile time
```

One drawback about the `std::initializer_list` implementation is that it constructs larger
`Vec`(size>4) of scalars or `vec`s at run time.

``` cpp
vtkm::Vec<vtkm::Float64, 5> vec1{1.1, 2.2, 3.3, 4.4, 5.5}; // Constructed at run time.

vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 5> vec2{{1.1, 1.1},{2.2, 2.2},{3.3, 3.3},
                                      {4.4, 4.4}, {5.5, 5.5}}; // Constructed at run time.
```

Parenthesis constructor would report an error if the size is larger than 4 when being used
to construct a `Vec` of `Vec`s. If it's being used to construct a `Vec` of scalars then it's
fine.

```cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 5> vec2({1.1, 1.1},{2.2, 2.2},{3.3, 3.3},
                                      {4.4, 4.4}, {5.5, 5.5}); // ERROR! This type of
                                                               // constructor not implemented!

vtkm::Vec<vtkm::Float64, 5> vec1(1.1, 2.2, 3.3, 4.4, 5.5); // Constructed at compile time.
```

If a `vtkm::Vec` is initialized with a list of size one, then that one value is
replicated for all components.

``` cpp
vtkm::Vec<vtkm::Float64, 3> vec{1.1};  // vec gets [ 1.1, 1.1, 1.1 ]
```

This "scalar" initialization also works for `Vec` of `Vec`s.

``` cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec1{ { 1.1, 2.2 } };
// vec1 is [[1.1, 2.2], [1.1, 2.2], [1.1, 2.2]]

vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec2{ { 3.3}, { 4.4 }, { 5.5 } };
// vec2 is [[3.3, 3.3], [4.4, 4.4], [5.5, 5.5]]
```

`vtkm::make_Vec` is also updated to support an arbitrary number initial values which are
constructed at compile time.

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

`vtkm::make_Vec` does not accept an `std::initializer_list`,

``` cpp
// Creates a vtkm::Vec<vtkm::Float64, 3>
auto vec1 = vtkm::make_Vec<3>({1.1, 2.2, 3.3}); // ERROR

// Creates exactly the same thing but compiles
auto vec1 = vtkm::make_Vec<3>(1.1, 2.2, 3.3);
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
