## Fix clang compile issue with a missing tempolate arg list

Apparently, starting with LLVM clang version 20, if you use the `template`
keyword to highlight a sub-element, you have to provide a template argument
list. This is true even for a method where the template arguments can be
completely determined by the types of the arguments. Fix this problem by
providing an empty template arg list (so the compiler knows what is
templated but still figures out its own types).

Fixes #830
