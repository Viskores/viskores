# Remove templates from ControlSignature field tags

Previously, several of the `ControlSignature` tags had a template to
specify a type list. This was to specify potential valid value types for an
input array. The importance of this typelist was to limit the number of
code paths created when resolving a `vtkm::cont::VariantArrayHandle`
(formerly a `DynamicArrayHandle`). This (potentially) reduced the compile
time, the size of libraries/executables, and errors from unexpected types.

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

This script is at
[Utilities/Scripts/update-control-signature-tags.sh](../../Utilities/Scripts/update-control-signature-tags.sh).
It needs to be run in a Unix-compatible shell. It takes a single argument,
which is a top level directory to modify files. The script processes all C++
source files recursively from that directory.

## Selecting data types for auxiliary filter fields

The main rational for making these changes is that the types of the inputs
to worklets is almost always already determined by the calling filter.
However, although it is straightforward to specify the type of the "main"
(active) scalars in a filter, it is less clear what to do for additional
fields if a filter needs a second or third field.

Typically, in the case of a second or third field, it is up to the
`DoExecute` method in the filter implementation to apply a policy to that
field. When applying a policy, you give it a policy object (nominally
passed by the user) and a traits of the filter. Generally, the accepted
list of types for a field should be part of the filter's traits. For
example, consider the `WarpVector` filter. This filter only works on
`Vec`s of size 3, so its traits class looks like this.

``` cpp
template <>
class FilterTraits<WarpVector>
{
public:
  // WarpVector can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
```

However, the `WarpVector` filter also requires two fields instead of one.
The first (active) field is handled by its superclass (`FilterField`), but
the second (auxiliary) field must be managed in the `DoExecute`. Generally,
this can be done by simply applying the policy with the filter traits.

## The corner cases

Most of the calls to worklets happen within filter implementations, which
have their own way of narrowing down potential types (as previously
described). The majority of the remainder either use static types or work
with a variety of types.

However, there is a minority of corner cases that require a reduction of
types. Since the type argument of the worklet `ControlSignature` arguments
are no longer available, the narrowing of types must be done before the
call to `Invoke`.

This narrowing of arguments is not particularly difficult. Such type-unsure
arguments usually come from a `VariantArrayHandle` (or something that uses
one). You can select the types from a `VariantArrayHandle` simply by using
the `ResetTypes` method. For example, say you know that a variant array is
supposed to be a scalar.

``` cpp
dispatcher.Invoke(variantArray.ResetTypes(vtkm::TypeListTagFieldScalar()),
                  staticArray);
```

Even more common is to have a `vtkm::cont::Field` object. A `Field` object
internally holds a `VariantArrayHandle`, which is accessible via the
`GetData` method.

``` cpp
dispatcher.Invoke(field.GetData().ResetTypes(vtkm::TypeListTagFieldScalar()),
                  staticArray);
```

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

After the changes, the executable sizes are as follows.

```
3.0M    libvtkm_cont-1.2.1.dylib
6.0M    libvtkm_rendering-1.2.1.dylib
312K    Rendering_SERIAL
312K    Rendering_TBB
 21M    Worklets_SERIAL
 21M    Worklets_TBB
 22M    UnitTests_vtkm_filter_testing
5.6M    UnitTests_vtkm_cont_serial_testing
6.0M    UnitTests_vtkm_cont_tbb_testing
7.1M    UnitTests_vtkm_cont_testing
```

As we can see, the built sizes have not changed significantly. (If
anything, the build is a little smaller.)
