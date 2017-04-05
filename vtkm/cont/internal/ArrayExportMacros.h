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

#define VTKM_BUILD_PREPARE_FOR_DEVICE
//Todo: This has been added to show the overhead cost of transporting
//simple arrays over to the most basic execution environment ( serial ). When
//we enable this we go from an original binary size of 356K to 702K.
//
//Now adding these symbols to the vtkm_cont library is really nice, because
//everyone downstream is using these in every translation unit and making
//them have only a single instantiation will pay off in the long run.
//
//So lets first go over what is happening when we move an ArrayHandle over
//to the serial backend, and that should explain why a doubling in the library
//size seems like overkill given the problem.
//
// PrepareForInput:
// Goal:
//   We need to verify that we previously haven't used this array with a
//   different device. If we have, we need to sync the host memory. Once
//   that is completed we need to wrap the host data pointer in a
//   ArrayPortalFromIterators and return that
//
// Problems:
//   We currently use an unique_ptr<ExecutionArray> to keep track of what
//   was the last device we uploaded onto. This is problematic as the ExecutionArray
//   is actually a full class with virtual methods, and can easily be replaced by
//   a simple struct which holds:
//   - DeviceAdapterId
//   - Type* of the memory returned
//   - Size of the memory
//
//   Basically this means instead of having a class that handles these functions
//   we instead use free functions that do the same, allowing us to remove
//   new/delete calls, and lots of class code generation.
//
// Notes:
//   If this is done properly can we refactor the std::shared_ptr in ArrayHandle
//   to have the same type no matter the valuetype past in? This would allow
//   us to again reduce the amount of code gen

#ifdef VTKM_BUILD_PREPARE_FOR_DEVICE

/// Declare extern template instantiations for all ArrayHandle transfer
/// infrastructure from a header file.
#define EXPORT_ARRAYHANDLE_DEVICE_ADAPTER(Type, Device) \
namespace internal { \
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayManagerExecution<Type, vtkm::cont::StorageTagBasic, Device>; \
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayManagerExecution<vtkm::Vec<Type,2>, vtkm::cont::StorageTagBasic, Device>; \
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayManagerExecution<vtkm::Vec<Type,3>, vtkm::cont::StorageTagBasic, Device>; \
extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayManagerExecution<vtkm::Vec<Type,4>, vtkm::cont::StorageTagBasic, Device>; \
} \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForInput(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForInput(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForInput(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForInPlace(Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForInPlace(Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForInPlace(Device); \
extern template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForDevice(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForDevice(Device) const; \
extern template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForDevice(Device) const;

/// Instantiate templates for all ArrayHandle transfer infrastructure from an
/// implementation file.
#define IMPORT_ARRAYHANDLE_DEVICE_ADAPTER(Type, Device) \
namespace internal { \
template class \
  ArrayManagerExecution<Type, vtkm::cont::StorageTagBasic, Device>; \
template class \
  ArrayManagerExecution<vtkm::Vec<Type,2>, vtkm::cont::StorageTagBasic, Device>; \
template class \
  ArrayManagerExecution<vtkm::Vec<Type,3>, vtkm::cont::StorageTagBasic, Device>; \
template class \
  ArrayManagerExecution<vtkm::Vec<Type,4>, vtkm::cont::StorageTagBasic, Device>; \
} \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForInput(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForInput(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::PortalConst \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForInput(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForOutput(vtkm::Id,Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForInPlace(Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForInPlace(Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
typename ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::ExecutionTypes<Device>::Portal \
ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForInPlace(Device); \
template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,2>, StorageTagBasic>::PrepareForDevice(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,3>, StorageTagBasic>::PrepareForDevice(Device) const; \
template VTKM_CONT_TEMPLATE_EXPORT \
void ArrayHandle<vtkm::Vec<Type,4>, StorageTagBasic>::PrepareForDevice(Device) const;

#include <vtkm/cont/ArrayHandle.h>

#endif // VTKM_BUILD_PREPARE_FOR_DEVICE

#endif // vtk_m_cont_internal_ArrayExportMacros_h
