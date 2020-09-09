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

template class VTKM_CONT_EXPORT ArrayHandleNewStyle<char, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Int8, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::UInt8, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Int16, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::UInt16, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Int32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::UInt32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Int64, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::UInt64, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Float32, StorageTagStride>;
template class VTKM_CONT_EXPORT ArrayHandleNewStyle<vtkm::Float64, StorageTagStride>;

}
} // namespace vtkm::cont
