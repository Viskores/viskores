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
  Algorithm.h
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
  ArrayHandleVirtualCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayHandleConcatenate.h
  ArrayRangeCompute.h
  ArrayRangeCompute.hxx
  AssignerMultiBlock.h
  CellLocator.h
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
  DecomposerMultiBlock.h
  DeviceAdapter.h
  DeviceAdapterAlgorithm.h
  DeviceAdapterListTag.h
  DynamicArrayHandle.h
  DynamicCellSet.h
  EnvironmentTracker.h
  Error.h
  ErrorBadAllocation.h
  ErrorBadDevice.h
  ErrorBadType.h
  ErrorBadValue.h
  ErrorExecution.h
  ErrorInternal.h
  Field.h
  ImplicitFunctionHandle.h
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
  VirtualObjectHandle.h
  )

set(template_sources
  ArrayHandle.hxx
  CellSetExplicit.hxx
  CellSetStructured.hxx
  StorageBasic.hxx
  )

set(sources
  ArrayHandle.cxx
  AssignerMultiBlock.cxx
  CellSet.cxx
  CellSetExplicit.cxx
  CellSetStructured.cxx
  DynamicArrayHandle.cxx
  EnvironmentTracker.cxx
  Field.cxx
  internal/SimplePolymorphicContainer.cxx
  internal/ArrayManagerExecutionShareWithControl.cxx
  StorageBasic.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayRangeCompute.cxx
  CellSetExplicit.cxx
  CoordinateSystem.cxx
  DataSet.cxx
  MultiBlock.cxx
  RuntimeDeviceTracker.cxx
  TryExecute.cxx
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

set(backends )
add_subdirectory(serial)
list(APPEND sources $<TARGET_OBJECTS:vtkm_cont_serial>)

add_subdirectory(tbb)
if(TARGET vtkm::tbb)
  list(APPEND sources $<TARGET_OBJECTS:vtkm_cont_tbb>)
  list(APPEND backends vtkm::tbb)
endif()

add_subdirectory(cuda)
if(TARGET vtkm::cuda)
  list(APPEND sources $<TARGET_OBJECTS:vtkm_cont_cuda>)
  list(APPEND backends vtkm::cuda)
endif()

vtkm_library( NAME vtkm_cont
              SOURCES ${sources}
              TEMPLATE_SOURCES ${template_sources}
              HEADERS ${headers}
              WRAP_FOR_CUDA ${device_sources}
            )
target_link_libraries(vtkm_cont PUBLIC vtkm_compiler_flags ${backends})
if(VTKm_ENABLE_MPI)
  # This will become a required dependency eventually.
  target_link_libraries(vtkm_cont PUBLIC diy)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(testing)
