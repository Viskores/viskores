//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
#define vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h

#include <vtkm/cont/internal/ArrayExportMacros.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <vtkm/cont/serial/internal/ExecutionArrayInterfaceBasicSerial.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagSerial>
  : public vtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>
{
public:
  using Superclass = vtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;

  VTKM_CONT
  ArrayManagerExecution(typename Superclass::StorageType* storage)
    : Superclass(storage)
  {
  }
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagSerial>
  : public ExecutionPortalFactoryBasicShareWithControl<T>
{
  using Superclass = ExecutionPortalFactoryBasicShareWithControl<T>;

  using typename Superclass::ValueType;
  using typename Superclass::PortalType;
  using typename Superclass::PortalConstType;
  using Superclass::CreatePortal;
  using Superclass::CreatePortalConst;
};

} // namespace internal

#ifndef vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_cxx
VTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagSerial)
#endif // !vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_cxx
}
} // namespace vtkm::cont

#endif //vtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
