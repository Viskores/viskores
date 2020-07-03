# Deprecate Execute with policy

The version of `Filter::Execute` that takes a policy as an argument is now
deprecated. Filters are now able to specify their own fields and types,
which is often why you want to customize the policy for an execution. The
other reason is that you are compiling VTK-m into some other source that
uses a particular types of storage. However, there is now a mechanism in
the CMake configuration to allow you to provide a header that customizes
the "default" types used in filters. This is a much more convenient way to
compile filters for specific types.

One thing that filters were not able to do was to customize what cell sets
they allowed using. This allows filters to self-select what types of cell
sets they support (beyond simply just structured or unstructured). To
support this, the lists `SupportedCellSets`, `SupportedStructuredCellSets`,
and `SupportedUnstructuredCellSets` have been added to `Filter`. When you
apply a policy to a cell set, you now have to also provide the filter.
