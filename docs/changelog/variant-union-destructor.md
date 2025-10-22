## Fixed variant compilation for SYCL

The SYCL compilation was running into a compile error with the destructor for
`VariantUnionNTD`. The following compile error was issued.

```
viskores/internal/VariantImplDetail.h:232:19: error: SYCL kernel cannot call an undefined function without SYCL_EXTERNAL attribute
232 | VISKORES_DEVICE ~VariantUnionNTD() { }
```

This is a strange error as the `VISKORES_DEVICE` should contain the method
modifiers needed. Nevertheless, the problem can be solved by changing the
destructor to be `default`.

However, many compilers have a problem with default destructors. For very in the
weeds reasons, the compiler removes the default destructor and causes subsequent
compile issues. Using an explicit destructor seems like the "right" thing to do,
but a special exception for SYCL compilations with C++20 and above uses a
default destructor.
