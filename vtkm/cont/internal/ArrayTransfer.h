//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayTransfer_h
#define vtk_m_cont_internal_ArrayTransfer_h

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/Token.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief Class that manages the transfer of data between control and execution (obsolete).
///
template <typename T, class StorageTag, class DeviceAdapterTag>
class ArrayTransfer
{
  VTKM_STATIC_ASSERT_MSG(sizeof(T) == static_cast<std::size_t>(-1),
                         "Default implementation of ArrayTransfer no longer available.");
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayTransfer_h
