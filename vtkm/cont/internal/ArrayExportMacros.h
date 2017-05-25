//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_internal_ArrayExportMacros_h
#define vtk_m_cont_internal_ArrayExportMacros_h

/// Declare extern template instantiations for all ArrayHandle transfer
/// infrastructure from a header file.
#define VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(Type, Device)                                   \
  namespace internal                                                                               \
  {                                                                                                \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayManagerExecution<Type, vtkm::cont::StorageTagBasic, Device>;                              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;                \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;                \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;                \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayTransfer<Type, vtkm::cont::StorageTagBasic, Device>;                                      \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayTransfer<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;                        \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayTransfer<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;                        \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayTransfer<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;                        \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleExecutionManager<Type, vtkm::cont::StorageTagBasic, Device>;                        \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;          \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;          \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;          \
  }                                                                                                \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst          \
      ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForInput(Device) const;             \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst          \
      ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForInput(Device) const;             \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst          \
      ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForInput(Device) const;             \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);        \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);        \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);        \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);        \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template VTKM_CONT_TEMPLATE_EXPORT                                                        \
    ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<Device>::Portal               \
      ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template VTKM_CONT_TEMPLATE_EXPORT void                                                   \
    ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(Device) const;                            \
  extern template VTKM_CONT_TEMPLATE_EXPORT void                                                   \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForDevice(Device) const;              \
  extern template VTKM_CONT_TEMPLATE_EXPORT void                                                   \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForDevice(Device) const;              \
  extern template VTKM_CONT_TEMPLATE_EXPORT void                                                   \
    ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForDevice(Device) const;

/// call VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER for all vtkm types.
#define VTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(Device)                                        \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(char, Device)                                         \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int8, Device)                                   \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt8, Device)                                  \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int16, Device)                                  \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt16, Device)                                 \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int32, Device)                                  \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt32, Device)                                 \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int64, Device)                                  \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt64, Device)                                 \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Float32, Device)                                \
  VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Float64, Device)

/// Instantiate templates for all ArrayHandle transfer infrastructure from an
/// implementation file.
#define VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(Type, Device)                              \
  namespace internal                                                                               \
  {                                                                                                \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayManagerExecution<Type, vtkm::cont::StorageTagBasic, Device>;                              \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;                \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;                \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayManagerExecution<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;                \
  template class VTKM_CONT_EXPORT ArrayTransfer<Type, vtkm::cont::StorageTagBasic, Device>;        \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayTransfer<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;                        \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayTransfer<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;                        \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayTransfer<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;                        \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayHandleExecutionManager<Type, vtkm::cont::StorageTagBasic, Device>;                        \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 2>, vtkm::cont::StorageTagBasic, Device>;          \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 3>, vtkm::cont::StorageTagBasic, Device>;          \
  template class VTKM_CONT_EXPORT                                                                  \
    ArrayHandleExecutionManager<vtkm::Vec<Type, 4>, vtkm::cont::StorageTagBasic, Device>;          \
  }                                                                                                \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<                    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<      \
    Device>::PortalConst ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForInput(Device) \
    const;                                                                                         \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<      \
    Device>::PortalConst ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForInput(Device) \
    const;                                                                                         \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<      \
    Device>::PortalConst ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForInput(Device) \
    const;                                                                                         \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);                        \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForOutput(vtkm::Id,   \
                                                                                       Device);    \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForOutput(vtkm::Id,   \
                                                                                       Device);    \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForOutput(vtkm::Id,   \
                                                                                       Device);    \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                                 \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForInPlace(Device);   \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForInPlace(Device);   \
  template VTKM_CONT_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::ExecutionTypes<      \
    Device>::Portal ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForInPlace(Device);   \
  template VTKM_CONT_EXPORT void ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(Device)      \
    const;                                                                                         \
  template VTKM_CONT_EXPORT void                                                                   \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>::PrepareForDevice(Device) const;              \
  template VTKM_CONT_EXPORT void                                                                   \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>::PrepareForDevice(Device) const;              \
  template VTKM_CONT_EXPORT void                                                                   \
    ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>::PrepareForDevice(Device) const;

/// call VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER for all vtkm types.
#define VTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(Device)                                   \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(char, Device)                                    \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int8, Device)                              \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt8, Device)                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int16, Device)                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt16, Device)                            \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int32, Device)                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt32, Device)                            \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Int64, Device)                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::UInt64, Device)                            \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Float32, Device)                           \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(vtkm::Float64, Device)

#include <vtkm/cont/ArrayHandle.h>

#endif // vtk_m_cont_internal_ArrayExportMacros_h
