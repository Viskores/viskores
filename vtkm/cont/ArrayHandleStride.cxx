//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtk_m_cont_ArrayHandleStride_cxx
#include <vtkm/cont/ArrayHandleStride.h>

#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template class VTKM_CONT_EXPORT Storage<char, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int8, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt8, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int16, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt16, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int32, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt32, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Int64, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::UInt64, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Float32, StorageTagStride>;
template class VTKM_CONT_EXPORT Storage<vtkm::Float64, StorageTagStride>;

} // namespace internal

template class VTKM_CONT_EXPORT ArrayHandle<char, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int8, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt8, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int16, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt16, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int64, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt64, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Float32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Float64, StorageTagStride>;

}
} // namespace vtkm::cont
