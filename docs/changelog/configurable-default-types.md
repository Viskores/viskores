# Configurable default types

Because VTK-m compiles efficient code for accelerator architectures, it
often has to compile for static types. This means that dynamic types often
have to be determined at runtime and converted to static types. This is the
reason for the `CastAndCall` architecture in VTK-m.

For this `CastAndCall` to work, there has to be a finite set of static
types to try at runtime. If you don't compile in the types you need, you
will get runtime errors. However, the more types you compile in, the longer
the compile time and executable size. Thus, getting the types right is
important.

The "right" types to use can change depending on the application using
VTK-m. For example, when VTK links in VTK-m, it needs to support lots of
types and can sacrifice the compile times to do so. However, if using VTK-m
in situ with a fortran simulation, space and time are critical and you
might only need to worry about double SoA arrays.

Thus, it is important to customize what types VTK-m uses based on the
application. This leads to the oxymoronic phrase of configuring the default
types used by VTK-m.

This is being implemented by providing VTK-m with a header file that
defines the default types. The header file provided to VTK-m should define
one or more of the following preprocessor macros:

  * `VTKM_DEFAULT_TYPE_LIST` - a `vtkm::List` of value types for fields that
     filters should directly operate on (where applicable).
  * `VTKM_DEFAULT_STORAGE_LIST` - a `vtkm::List` of storage tags for fields
     that filters should directly operate on.
  * `VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED` - a `vtkm::List` of
     `vtkm::cont::CellSet` types that filters should operate on as a
     strutured cell set.
  * `VTKM_DEFAULT_CELL_SET_LIST_UNSTRUCTURED` - a `vtkm::List` of
     `vtkm::cont::CellSet` types that filters should operate on as an
     unstrutured cell set.
  * `VTKM_DEFAULT_CELL_SET_LIST` - a `vtkm::List` of `vtkm::cont::CellSet`
     types that filters should operate on (where applicable). The default of
     `vtkm::ListAppend<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED, VTKM_DEFAULT_CELL_SET_LIST>`
	 is usually correct.

If any of these macros are not defined, a default version will be defined.
(This is the same default used if no header file is provided.)

This header file is provided to the build by setting the
`VTKm_DEFAULT_TYPES_HEADER` CMake variable. `VTKm_DEFAULT_TYPES_HEADER`
points to the file, which will be configured and copied to VTK-m's build
directory.

For convenience, header files can be added to the VTK_m source directory
(conventionally under vtkm/cont/internal). If this is the case, an advanced
CMake option should be added to select the provided header file.
