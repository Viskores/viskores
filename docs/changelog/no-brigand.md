# No longer use brigand.hpp

Deprecate the use of brigand.hpp both within VTK-m and for any code that
might use VTK-m's distribution of brigand.hpp.

Brigand is a third-party library to support template meta-programming. Over
the years, we have had to make a few modifications to make sure it compiles
with all compilers supported by VTK-m. Unfortunately, because brigand was
added before our standard third-party library management was set up, these
changes are not managed well. Thus, we cannot easily update with any
changes from the project. Thus, our version is slowly diverging from the
original, and maintaining it is a hassle.

Also, we have been using brigand less and less throughout the years. Now
that we have moved on to C++11 (and now C++14) with variadic templates and
other useful `std` features, the features of brigand have become less
critical. Thus, we have implemented all the features we need from brigand
internally and have moved our code away from using it.
