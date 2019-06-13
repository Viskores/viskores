//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtkm_cont_ArrayHandle_cxx
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{

#define VTKM_ARRAYHANDLE_INSTANTIATE(Type)                                                         \
  template class VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>;                              \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>;                \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>;                \
  template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>;

VTKM_ARRAYHANDLE_INSTANTIATE(char)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Int8)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::UInt8)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Int16)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::UInt16)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Int32)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::UInt32)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Int64)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::UInt64)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Float32)
VTKM_ARRAYHANDLE_INSTANTIATE(vtkm::Float64)

#undef VTKM_ARRAYHANDLE_INSTANTIATE
}
} // end vtkm::cont
