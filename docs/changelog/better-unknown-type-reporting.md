# Improve type reporting in `UnknownArrayHandle`

Added features with reporting types with `UnknownArrayHandle`. First, added
a method named `GetArrayTypeName` that returns a string containing the type
of the contained array. There were already methods `GetValueType` and
`GetStorageType`, but this provides a convenience to get the whole name in
one go.

Also improved the reporting when an `AsArrayHandle` call failed. Before,
the thrown method just reported that the `UnknownArrayHandle` could not be
converted to the given type. Now, it also reports the type actually held by
the `UnknownArrayHandle` so the user can better understand why the
conversion failed.

