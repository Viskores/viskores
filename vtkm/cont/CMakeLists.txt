##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  Algorithm.h
  ArrayCopy.h
  ArrayGetValues.h
  ArrayHandle.h
  ArrayHandleBitField.h
  ArrayHandleCartesianProduct.h
  ArrayHandleCast.h
  ArrayHandleCompositeVector.h
  ArrayHandleConcatenate.h
  ArrayHandleConstant.h
  ArrayHandleCounting.h
  ArrayHandleDecorator.h
  ArrayHandleDiscard.h
  ArrayHandleExtractComponent.h
  ArrayHandleExtrudeCoords.h
  ArrayHandleExtrudeField.h
  ArrayHandleGroupVec.h
  ArrayHandleGroupVecVariable.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandleMultiplexer.h
  ArrayHandlePermutation.h
  ArrayHandleReverse.h
  ArrayHandleSOA.h
  ArrayHandleStreaming.h
  ArrayHandleSwizzle.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleView.h
  ArrayHandleVirtual.h
  ArrayHandleVirtualCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayRangeCompute.h
  AssignerPartitionedDataSet.h
  AtomicArray.h
  BitField.h
  BoundsCompute.h
  BoundsGlobalCompute.h
  CastAndCall.h
  CellLocator.h
  CellLocatorBoundingIntervalHierarchy.h
  CellLocatorGeneral.h
  CellLocatorRectilinearGrid.h
  CellLocatorUniformBins.h
  CellLocatorUniformGrid.h
  CellSet.h
  CellSetExplicit.h
  CellSetExtrude.h
  CellSetList.h
  CellSetListTag.h
  CellSetPermutation.h
  CellSetSingleType.h
  CellSetStructured.h
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
  DeviceAdapterList.h
  DeviceAdapterListTag.h
  DeviceAdapterTag.h
  DynamicCellSet.h
  EnvironmentTracker.h
  Error.h
  ErrorBadAllocation.h
  ErrorBadDevice.h
  ErrorBadType.h
  ErrorBadValue.h
  ErrorExecution.h
  ErrorFilterExecution.h
  ErrorInternal.h
  ExecutionAndControlObjectBase.h
  ExecutionObjectBase.h
  Field.h
  FieldRangeCompute.h
  FieldRangeGlobalCompute.h
  ImplicitFunctionHandle.h
  Initialize.h
  Invoker.h
  Logging.h
  PartitionedDataSet.h
  PointLocator.h
  PointLocatorUniformGrid.h
  RuntimeDeviceInformation.h
  RuntimeDeviceTracker.h
  Serialization.h
  Storage.h
  StorageBasic.h
  StorageExtrude.h
  StorageImplicit.h
  StorageList.h
  StorageListTag.h
  StorageVirtual.h
  Timer.h
  TryExecute.h
  SerializableTypeString.h
  VariantArrayHandle.h
  VirtualObjectHandle.h
  )

set(template_sources
  ArrayHandle.hxx
  ArrayHandleVirtual.hxx
  ArrayRangeCompute.hxx
  CellSetExplicit.hxx
  CellSetExtrude.hxx
  CellSetStructured.hxx
  ColorTable.hxx
  CoordinateSystem.hxx
  FieldRangeCompute.hxx
  FieldRangeGlobalCompute.hxx
  StorageBasic.hxx
  StorageVirtual.hxx
  VirtualObjectHandle.hxx
  )

set(sources
  ArrayHandle.cxx
  ArrayHandleSOA.cxx
  ColorTablePresets.cxx
  DeviceAdapterTag.cxx
  EnvironmentTracker.cxx
  ErrorBadDevice.cxx
  ErrorBadType.cxx
  internal/ArrayHandleBasicImpl.cxx
  internal/ArrayManagerExecutionShareWithControl.cxx
  internal/TransferInfo.cxx
  internal/VirtualObjectTransfer.cxx
  Initialize.cxx
  Logging.cxx
  RuntimeDeviceTracker.cxx
  StorageBasic.cxx
  TryExecute.cxx
  )

  # This list of sources has code that uses devices and so might need to be
  # compiled with a device-specific compiler (like CUDA).
  set(device_sources
  ArrayHandleVirtual.cxx
  ArrayRangeCompute.cxx
  AssignerPartitionedDataSet.cxx
  BoundsCompute.cxx
  BoundsGlobalCompute.cxx
  CellLocator.cxx
  CellLocatorBoundingIntervalHierarchy.cxx
  CellLocatorGeneral.cxx
  CellLocatorRectilinearGrid.cxx
  CellLocatorUniformBins.cxx
  CellLocatorUniformGrid.cxx
  CellSet.cxx
  CellSetExplicit.cxx
  CellSetExtrude.cxx
  CellSetStructured.cxx
  ColorTable.cxx
  CoordinateSystem.cxx
  DataSet.cxx
  DataSetBuilderExplicit.cxx
  DataSetBuilderRectilinear.cxx
  DataSetBuilderUniform.cxx
  internal/VariantArrayHandleContainer.cxx
  Field.cxx
  FieldRangeCompute.cxx
  FieldRangeGlobalCompute.cxx
  PartitionedDataSet.cxx
  PointLocator.cxx
  PointLocatorUniformGrid.cxx
  RuntimeDeviceInformation.cxx
  StorageVirtual.cxx
  Timer.cxx
  )

#-----------------------------------------------------------------------------
vtkm_library( NAME vtkm_cont
              SOURCES ${sources}
              TEMPLATE_SOURCES ${template_sources}
              HEADERS ${headers}
              DEVICE_SOURCES ${device_sources}
            )

add_subdirectory(internal)
add_subdirectory(arg)
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
target_link_libraries(vtkm_cont PUBLIC vtkm_taotuple vtkm_optionparser vtkm_diy vtkm_lcl)
if(TARGET vtkm_loguru)
  target_link_libraries(vtkm_cont PRIVATE vtkm_loguru)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(testing)
