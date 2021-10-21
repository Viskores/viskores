# Filters instantiation generator

It introduces a template instantiation generator. This aims to significantly
reduce the memory usage when building _VTK-m_ filter by effectively splitting
templates instantiations across multiple files.

How this works revolves around automatically instantiating filters template
methods inside transient instantiation files which resides solely in the build
directory. Each of those transient files contains a single explicit template
instantiation.

Here is an example of how to produce an instantiation file.

First, at the filter header file:

```c++

// 1. Include Instantiations header
#include <vtkm/filter/Instantiations.h>

class Contour {
  template <typename T, typename StorageType, typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet&,
                                const vtkm::cont::ArrayHandle<T, StorageType>&,
                                const vtkm::filter::FieldMetadata&,
                                vtkm::filter::PolicyBase<DerivedPolicy>);
};

// 2. Create extern template instantiation and surround with
//    VTKM_INSTANTIATION_{BEGIN,END}
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

```

Later, in its corresponding `CMakeLists.txt` file:

```cmake

vtkm_add_instantiations(ContourInstantiations FILTER Contour)
vtkm_library(
  NAME vtkm_filter_contour
  ...
  DEVICE_SOURCES ${ContourInstantiations}
  )

```

After running the configure step in _CMake_, this will result in the creation of
the following transient file in the build directory:

```c++

#ifndef vtkm_filter_ContourInstantiation0_cxx
#define vtkm_filter_ContourInstantiation0_cxx
#endif

/* Needed for linking errors when no instantiations */
int __vtkm_filter_ContourInstantiation0_cxx;

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Contour.hxx>

namespace vtkm
{
namespace filter
{

template vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

}
}

#undef vtkm_filter_ContourInstantiation0_cxx

```
