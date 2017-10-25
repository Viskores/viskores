##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayCopy.h
  ArrayHandle.h
  ArrayHandleCast.h
  ArrayHandleCartesianProduct.h
  ArrayHandleCompositeVector.h
  ArrayHandleConstant.h
  ArrayHandleCounting.h
  ArrayHandleExtractComponent.h
  ArrayHandleDiscard.h
  ArrayHandleGroupVec.h
  ArrayHandleGroupVecVariable.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandlePermutation.h
  ArrayHandleReverse.h
  ArrayHandleStreaming.h
  ArrayHandleSwizzle.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayHandleConcatenate.h
  ArrayRangeCompute.h
  ArrayRangeCompute.hxx
  CellLocatorTwoLevelUniformGrid.h
  CellSet.h
  CellSetExplicit.h
  CellSetListTag.h
  CellSetSingleType.h
  CellSetStructured.h
  CellSetPermutation.h
  CoordinateSystem.h
  DataSet.h
  DataSetBuilderExplicit.h
  DataSetBuilderRectilinear.h
  DataSetBuilderUniform.h
  DataSetFieldAdd.h
  DeviceAdapter.h
  DeviceAdapterAlgorithm.h
  DeviceAdapterListTag.h
  DynamicArrayHandle.h
  DynamicCellSet.h
  Error.h
  ErrorBadAllocation.h
  ErrorBadType.h
  ErrorBadValue.h
  ErrorExecution.h
  ErrorInternal.h
  Field.h
  ImplicitFunction.h
  MultiBlock.h
  PointLocatorUniformGrid.h
  RuntimeDeviceInformation.h
  RuntimeDeviceTracker.h
  Storage.h
  StorageBasic.h
  StorageImplicit.h
  StorageListTag.h
  Timer.h
  TryExecute.h
  VirtualObjectCache.h
  )

set(header_impls
  ArrayHandle.hxx
  CellSetExplicit.hxx
  CellSetStructured.hxx
  ImplicitFunction.hxx
  StorageBasic.hxx
  )

set(sources
  ArrayHandle.cxx
  CellSet.cxx
  CellSetExplicit.cxx
  CellSetStructured.cxx
  CoordinateSystem.cxx
  DynamicArrayHandle.cxx
  Field.cxx
  ImplicitFunction.cxx
  internal/SimplePolymorphicContainer.cxx
  MultiBlock.cxx
  internal/ArrayManagerExecutionShareWithControl.cxx
  StorageBasic.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayRangeCompute.cxx
  CellSetExplicit.cxx
  RuntimeDeviceTracker.cxx
  )

vtkm_declare_headers(${headers})

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

add_subdirectory(serial)
set(sources ${sources} $<TARGET_OBJECTS:vtkm_cont_serial>)

add_subdirectory(tbb)
if (VTKm_ENABLE_TBB)
  set(sources ${sources} $<TARGET_OBJECTS:vtkm_cont_tbb>)
endif()

add_subdirectory(cuda)
if (VTKm_ENABLE_CUDA)
  get_property(vtkm_cont_cuda_object_files GLOBAL
               PROPERTY vtkm_cont_cuda_object_files )
  # mark the file as generated, this needs to be done in this directory
  # for 'reasons'.
  set_source_files_properties(${vtkm_cont_cuda_object_files}
                              PROPERTIES GENERATED TRUE)

  set(sources ${sources} ${vtkm_cont_cuda_object_files})
endif()

vtkm_library(
  SOURCES ${sources}
  WRAP_FOR_CUDA ${device_sources}
  HEADERS ${header_impls})
target_include_directories(vtkm_cont PRIVATE ${VTKm_BACKEND_INCLUDE_DIRS})

if(VTKm_ENABLE_CUDA)
  add_dependencies(vtkm_cont vtkm_cont_cuda)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(testing)
