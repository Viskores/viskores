## Checking of class triviality is improved

When Viskores packages classes to send between host and device as well as when
Viskores usings Variants to build unions of objects, it can be important to
check trivial aspects of classes. For example, it is sometimes important to know
whether the class executes code for its constructors or destructor.

Early forms of Viskores (actually its predecessor VTK-m) used compilers that did
not give reliable results for `std::is_trivial` and related type checks. To get
around this problem on these compilers, Viskores implemented alternate forms of
these checks that reported nothing as trivial so that proper construction copy
or destruction happened. This alternate implementation also reported that
everything _was_ trivial during assert checks in places where objects needed to
be memory copied. (Other compilers were used to ensure that these checks passed
correctly.)

Now that Viskores requires C++17 or better, compilers _should_ properly
implement these checks that have been around since C++11. Thus, the workarounds
for bad implementations of `std::is_trivial` should not be necessary, and they
have been removed.
