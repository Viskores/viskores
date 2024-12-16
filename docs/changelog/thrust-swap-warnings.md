## Circumvent shadow warnings with thrust swap

We have run into issues with the `nvcc` compiler giving shadow warnings for
the internals of thrust like this:

```
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h: In constructor 'thrust::detail::unary_negate<Predicate>::unary_negate(const Predicate&)':
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h:45:46: warning: declaration of 'pred' shadows a member of 'thrust::detail::unary_negate<Predicate>' [-Wshadow]
   explicit unary_negate(const Predicate& pred) : pred(pred) {}
                                              ^
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h:42:11: note: shadowed declaration is here
   Predicate pred;
           ^
```

These warnings seem to be caused by the inclusion of `thrust/swap.h`. To
prevent this, this header file is no longer included from `vtkm/Swap.h`.
