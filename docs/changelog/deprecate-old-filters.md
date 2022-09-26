# Old Filter Base Classes are Deprecated

In recent versions of VTK-m, a new structure for filter classes was
introduced. All of the existing filters have been moved over to this new
structure, and the old filter class structure has been deprecated.

This is in preparation for changed in VTK-m 2.0, where the old filter
classes will be removed and the new filter classes will have the `New` in
their name removed (so that they become simply `Filter` and `FilterField`).
