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
  AssignerMultiBlock.h
  AtomicArray.h
  BoundingIntervalHierarchyNode.h
  BoundingIntervalHierarchy.h
  BoundsCompute.h
  BoundsGlobalCompute.h
  CellLocator.h
  CellLocatorHelper.h
  CellLocatorTwoLevelUniformGrid.h
  CellSet.h
  CellSetExplicit.h
  CellSetListTag.h
  CellSetSingleType.h
  CellSetStructured.h
  CellSetPermutation.h
  ColorTable.h
  ColorTableSamples.h
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
  EnvironmentTracker.h
  Error.h
  ErrorBadAllocation.h
  ErrorBadDevice.h
  ErrorBadType.h
  ErrorBadValue.h
  ErrorFilterExecution.h
  ErrorExecution.h
  ErrorInternal.h
  ExecutionObjectBase.h
  Field.h
  FieldRangeCompute.h
  FieldRangeGlobalCompute.h
  ImplicitFunctionHandle.h
  MultiBlock.h
  PointLocator.h
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
  ArrayRangeCompute.hxx
  BoundingIntervalHierarchy.hxx
  CellSetExplicit.hxx
  CellSetStructured.hxx
  CoordinateSystem.hxx
  FieldRangeCompute.hxx
  FieldRangeGlobalCompute.hxx
  StorageBasic.hxx
  )

set(sources
  ArrayHandle.cxx
  AssignerMultiBlock.cxx
  BoundsCompute.cxx
  BoundsGlobalCompute.cxx
  CellSet.cxx
  CellSetStructured.cxx
  DataSet.cxx
  DataSetBuilderExplicit.cxx
  DataSetBuilderRectilinear.cxx
  DataSetBuilderUniform.cxx
  DynamicArrayHandle.cxx
  EnvironmentTracker.cxx
  Field.cxx
  FieldRangeCompute.cxx
  FieldRangeGlobalCompute.cxx
  internal/ArrayHandleBasicImpl.cxx
  internal/ArrayManagerExecutionShareWithControl.cxx
  internal/SimplePolymorphicContainer.cxx
  MultiBlock.cxx
  PresetColorTables.cxx
  RuntimeDeviceTracker.cxx
  StorageBasic.cxx
  TryExecute.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayRangeCompute.cxx
  CellSetExplicit.cxx
  ColorTable.cxx
  CoordinateSystem.cxx
  )

#-----------------------------------------------------------------------------
vtkm_library( NAME vtkm_cont
              SOURCES ${sources}
              TEMPLATE_SOURCES ${template_sources}
              HEADERS ${headers}
              WRAP_FOR_CUDA ${device_sources}
            )
add_subdirectory(internal)
add_subdirectory(arg)
add_subdirectory(diy)
add_subdirectory(serial)
add_subdirectory(tbb)
add_subdirectory(openmp)
add_subdirectory(cuda)

set(backends )
if(TARGET vtkm::tbb)
  list(APPEND backends vtkm::tbb)
endif()
if(TARGET vtkm::cuda)
  list(APPEND backends vtkm::cuda)
endif()
if(TARGET vtkm::openmp)
  list(APPEND backends vtkm::openmp)
endif()
target_link_libraries(vtkm_cont PUBLIC vtkm_compiler_flags ${backends})
if(TARGET vtkm_diy)
  # This will become a required dependency eventually.
  target_link_libraries(vtkm_cont PUBLIC vtkm_diy vtkm_taotuple)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(testing)
