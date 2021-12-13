## New Filter Interface Design ##

An overhaul of the Filter interface is undergoing. This refactoring effort will
address many problems we faced in the old design. The most important one is to
remove the requirement to compile every single Filter users with a Device Compiler.
This is addressed by removing C++ template (and CRTP) from Filter and is subclasses.
A new non-templated NewFilter class is added with many old templated public interface
removed.

This new design also made Filter implementations thread-safe by default. Filter
implementations are encouraged to take advantage of the new design and removing
shared metatable states from their `DoExecute`, see Docygen documentation in
NewFilter.h

Filter implementations are also re-organized into submodules, with each submodule
in its own `vtkm/filter` subdirectory. User should update their code to include
the new header files, for example, `vtkm/filter/field_transform/GenerateIds.h`and
link to submodule library file, for example, `libvtkm_filter_field_transform.so`.
To maintain backward compatability, old `vtkm/filter/FooFilter.h` header files
can still be used but will be deprecated in release 2.0.
