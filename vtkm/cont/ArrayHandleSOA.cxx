//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtkm_cont_ArrayHandleSOA_cxx
#include <vtkm/cont/ArrayHandleSOA.h>

namespace vtkm
{
namespace cont
{

#define VTKM_ARRAYHANDLE_SOA_INSTANTIATE(Type)                                                     \
  template class VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagSOA>;                                \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagSOA>;                  \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagSOA>;                  \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagSOA>;

VTKM_ARRAYHANDLE_SOA_INSTANTIATE(char)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Int8)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::UInt8)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Int16)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::UInt16)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Int32)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::UInt32)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Int64)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::UInt64)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Float32)
VTKM_ARRAYHANDLE_SOA_INSTANTIATE(vtkm::Float64)

#undef VTKM_ARRAYHANDLE_SOA_INSTANTIATE
}
} // end vtkm::cont
