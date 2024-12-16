## Add constexpr to Vec methods

The `constexpr` keyword is helpful to add to functions and macros where
possible. Better than `inline`, it tells the compiler that it can perform
optimizations based on analysis of expressions and literals given in the
code. In particular, this should help code that loops over components have
proper optimizations like loop unrolling when using `Vec` types that have
the number of components fixed.
