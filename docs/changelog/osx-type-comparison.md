# Fix type comparison on OSX

`UnknownArrayHandle` compares `std::type_index` objects to check whether a
requested type is the same as that held in the array handle. However, it is
possible that different translation units can create different but
equivalent `std::type_info`/`std::type_index` objects. In this case, the
`==` operator might return false for two equivalent types. This can happen
on OSX.

To get around this problem, `UnknownArrayHandle` now does a more extensive
check for `std::type_info` object. It first uses the `==` operator to
compare them (as before), which usually works but can possibly return
`false` when the correct result is `true`. To check for this case, it then
compares the name for the two types and returns `true` iff the two names
are the same.
