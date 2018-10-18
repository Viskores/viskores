# Remove templates from ControlSignature field tags

Previously, several of the `ControlSignature` tags had a template to specify a
type list. This was to specify potential valid value types for an input array.
The importance of this typelist was to limit the number of code paths created
when resolving a `vtkm::cont::DynamicArrayHandle`. This (potentially) reduced
the compile time, the size of libraries/executables, and errors from
unexpected types.

Much has changed since this feature was originally implemented. Since then,
the filter infrastructure has been created, and it is through this that
most dynamic worklet invocations happen. However, since the filter
infrastrcture does its own type resolution (and has its own policies) the
type arguments in `ControlSignature` are now of little value.

## Script to update code

This update requires changes to just about all code implementing a VTK-m
worklet. To facilitate the update of this code to these new changes (not to
mention all the code in VTK-m) a script is provided to automatically remove
these template parameters from VTK-m code.

*** Add information about script ***

## Change in executable size

The whole intention of these template parameters in the first place was to
reduce the number of code paths compiled. The hypothesis of this change was
that in the current structure the code paths were not being reduced much
if at all. If that is true, the size of executables and libraries should
not change.

Here is a recording of the library and executable sizes before this change
(using `ds -h`).

```
3.0M    libvtkm_cont-1.2.1.dylib
6.2M    libvtkm_rendering-1.2.1.dylib
312K    Rendering_SERIAL
312K    Rendering_TBB
 22M    Worklets_SERIAL
 23M    Worklets_TBB
 22M    UnitTests_vtkm_filter_testing
5.7M    UnitTests_vtkm_cont_serial_testing
6.0M    UnitTests_vtkm_cont_tbb_testing
7.1M    UnitTests_vtkm_cont_testing
```
