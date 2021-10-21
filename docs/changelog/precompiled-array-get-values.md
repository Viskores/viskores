# Compile `ArrayGetValues` implementation in a library

Previously, all of the `ArrayGetValue` implementations were templated
functions that had to be built by all code that used it. That had 2
negative consequences.

1. The same code that scheduled jobs on any device had to be compiled many
   times over.
2. Any code that used `ArrayGetValue` had to be compiled with a device
   compiler. If you had non-worklet code that just wanted to get a single
   value out of an array, that was a pain.
   
To get around this problem, an `ArrayGetValues` function that takes
`UnknownArrayHandle`s was created. The implementation for this function is
compiled into a library. It uses `UnknownArrayHandle`'s ability to extract
a component of the array with a uniform type to reduce the number of code
paths it generates. Although there are still several code paths, they only
have to be computed once. Plus, now any code can include `ArrayGetValues.h`
and still use a basic C++ compiler.

