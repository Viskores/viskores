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
#include <vtkm/cont/internal/ArrayManagerExecution.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief Class that manages the transfer of data between control and execution.
///
/// This templated class provides a mechanism (used by the ArrayHandle) to
/// transfer data from the control environment to the execution environment and
/// back. The interface for ArrayTransfer is nearly identical to that of
/// ArrayManagerExecution and the default implementation simply delegates all
/// calls to that class.
///
/// The primary motivation for having a separate class is that the
/// ArrayManagerExecution is meant to be specialized for each device adapter
/// whereas the ArrayTransfer is meant to be specialized for each storage type
/// (or specific combination of storage and device adapter). Thus, transfers
/// for most storage tyeps will be delegated through the ArrayManagerExecution,
/// but some storage types, like implicit storage, will be specialized to
/// transfer through a different path.
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
