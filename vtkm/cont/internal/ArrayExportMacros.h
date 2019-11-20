//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_internal_ArrayExportMacros_h
#define vtk_m_cont_internal_ArrayExportMacros_h

/// Declare extern template instantiations for all ArrayHandle transfer
/// infrastructure from a header file.
#define VTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(Type, Device)                    \
  namespace internal                                                                               \
  {                                                                                                \
  extern template struct VTKM_CONT_TEMPLATE_EXPORT ExecutionPortalFactoryBasic<Type, Device>;      \
  }                                                                                                \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);        \
  extern template VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template VTKM_CONT_TEMPLATE_EXPORT void                                                   \
  ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(const std::unique_lock<std::mutex>&,        \
                                                       Device) const;

#define VTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(BasicType, Device)                              \
  VTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(BasicType, Device)                     \
  VTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 2>), Device)                                             \
  VTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 3>), Device)                                             \
  VTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 4>), Device)

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
#define VTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(Type, Device)               \
  namespace internal                                                                               \
  {                                                                                                \
  template struct VTKM_CONT_EXPORT ExecutionPortalFactoryBasic<Type, Device>;                      \
  }                                                                                                \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<                    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id, Device);                        \
  template VTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                                 \
  template VTKM_CONT_EXPORT void ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(             \
    const std::unique_lock<std::mutex>&, Device) const;

#define VTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(BasicType, Device)                         \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(BasicType, Device)                \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 2>), Device)                                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 3>), Device)                                             \
  VTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    VTKM_PASS_COMMAS(vtkm::Vec<BasicType, 4>), Device)

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
