## Using fewer types for VTK

The list of value types is no longer a huge list for VTK types. Previously, it
was important to include many types in the default type list so that cast and
calls could capture all the possible arrays provided by VTK. However, recent
changes to Viskores mean that most filters have safe fallbacks to manage types
outside of this list. Thus, it should no longer be necessary to attempt to
manage every possible type.
