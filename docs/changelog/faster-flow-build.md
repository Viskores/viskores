## Improve compilation of flow filters

Several of the flow filters contain multiple flow paths through different
worklets. This can sometimes overwhelm device compilers. To improve compilation,
the compilation of worklets for the flow filters is separated into different
translation units using the Viskores instantiation compile feature. This reduces
the burden on any particular use of a compiler and helps leverage parallel
compiling.
