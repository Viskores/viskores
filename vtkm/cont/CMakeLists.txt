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
  ArrayExtractComponent.h
  ArrayGetValues.h
  ArrayHandle.h
  ArrayHandleBasic.h
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
  ArrayHandleGroupVec.h
  ArrayHandleGroupVecVariable.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandleMultiplexer.h
  ArrayHandleOffsetsToNumComponents.h
  ArrayHandlePermutation.h
  ArrayHandleRecombineVec.h
  ArrayHandleReverse.h
  ArrayHandleRandomStandardNormal.h
  ArrayHandleRandomUniformBits.h
  ArrayHandleRandomUniformReal.h
  ArrayHandleSOA.h
  ArrayHandleStride.h
  ArrayHandleSwizzle.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleView.h
  ArrayHandleXGCCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayRangeCompute.h
  ArrayRangeComputeTemplate.h
  AssignerPartitionedDataSet.h
  AtomicArray.h
  BitField.h
  BoundsCompute.h
  BoundsGlobalCompute.h
  CastAndCall.h
  CellLocatorBoundingIntervalHierarchy.h
  CellLocatorChooser.h
  CellLocatorGeneral.h
  CellLocatorRectilinearGrid.h
  CellLocatorTwoLevel.h
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
  ColorTableMap.h
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
  ParticleArrayCopy.h
  PartitionedDataSet.h
  PointLocatorUniformGrid.h
  PointLocatorSparseGrid.h
  RuntimeDeviceInformation.h
  RuntimeDeviceTracker.h
  Serialization.h
  Storage.h
  StorageImplicit.h # Deprecated, rolled into ArrayHandleImplicit.h
  StorageList.h
  StorageListTag.h
  Timer.h
  Token.h
  TryExecute.h
  SerializableTypeString.h
  UncertainArrayHandle.h
  UnknownArrayHandle.h
  VariantArrayHandle.h
  VirtualObjectHandle.h
  )

set(template_sources
  ArrayRangeCompute.hxx # Deprecated, replaced with ArrayRangeComputeTemplate.h
  CellSetExplicit.hxx
  ParticleArrayCopy.hxx
  VirtualObjectHandle.hxx
  )

set(sources
  ArrayHandle.cxx
  ArrayHandleBasic.cxx
  ArrayHandleSOA.cxx
  ArrayHandleStride.cxx
  ArrayHandleUniformPointCoordinates.cxx
  BitField.cxx
  ColorTablePresets.cxx
  DeviceAdapterTag.cxx
  EnvironmentTracker.cxx
  ErrorBadDevice.cxx
  ErrorBadType.cxx
  internal/Buffer.cxx
  internal/DeviceAdapterMemoryManager.cxx
  internal/DeviceAdapterMemoryManagerShared.cxx
  internal/TransferInfo.cxx
  internal/VirtualObjectTransfer.cxx
  Initialize.cxx
  Logging.cxx
  RuntimeDeviceTracker.cxx
  Storage.cxx
  Token.cxx
  TryExecute.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayCopy.cxx
  ArrayRangeCompute.cxx
  AssignerPartitionedDataSet.cxx
  BoundsCompute.cxx
  BoundsGlobalCompute.cxx
  CellLocatorBoundingIntervalHierarchy.cxx
  CellLocatorGeneral.cxx
  CellLocatorRectilinearGrid.cxx
  CellLocatorTwoLevel.cxx
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
  Field.cxx
  FieldRangeCompute.cxx
  FieldRangeGlobalCompute.cxx
  PartitionedDataSet.cxx
  PointLocatorSparseGrid.cxx
  RuntimeDeviceInformation.cxx
  Timer.cxx
  UnknownArrayHandle.cxx
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    ArrayHandleVirtual.h
    ArrayHandleVirtualCoordinates.h
    CellLocator.h
    PointLocator.h
    StorageVirtual.h
    )

  set(template_sources ${template_sources}
    ArrayHandleVirtual.hxx
    StorageVirtual.hxx
    )

  set(device_sources ${device_sources}
    ArrayHandleVirtual.cxx
    CellLocator.cxx
    PointLocator.cxx
    StorageVirtual.cxx
    )
endif()

#-----------------------------------------------------------------------------
# Set up default types, which can be custom configured by other build systems.
vtkm_get_kit_name(kit_name kit_dir)

# Some custom types included with VTK-m
vtkm_option(VTKm_USE_DEFAULT_TYPES_FOR_VTK
  "Compile VTK-m algorithms for use with types from VTK."
  OFF
  )
if (VTKm_USE_DEFAULT_TYPES_FOR_VTK)
  set(VTKm_DEFAULT_TYPES_HEADER "internal/DefaultTypesVTK.h.in")
  vtkm_option(VTKm_ADD_XGC_TO_DEFAULT_TYPES_VTK "Add XGC types to default VTK types." OFF)
  mark_as_advanced(VTKm_ADD_XGC_TO_DEFAULT_TYPES_VTK)
  if (VTKm_ADD_XGC_TO_DEFAULT_TYPES_VTK)
    set(VTKM_ADD_XGC_DEFAULT_TYPES TRUE)
  endif()
endif()

vtkm_option(VTKm_USE_DEFAULT_TYPES_FOR_ASCENT
  "Compile VTK-m algorithms for use with types from Ascent."
  OFF
  )
if (VTKm_USE_DEFAULT_TYPES_FOR_ASCENT)
  set(VTKm_DEFAULT_TYPES_HEADER "internal/DefaultTypesAscent.h.in")
  if (VTKm_USE_64BIT_IDS)
    message(SEND_ERROR
      "When VTKm_USE_DEFAULT_TYPES_FOR_ASCENT is ON, VTKm_USE_64BIT_IDS must be set to OFF"
      )
  endif()
  if (NOT VTKm_USE_DOUBLE_PRECISION)
    message(SEND_ERROR
      "When VTKm_USE_DEFAULT_TYPES_FOR_ASCENT is ON, VTKm_USE_DOUBLE_PRECISION must be set to ON"
      )
  endif()
endif()

mark_as_advanced(
  VTKm_USE_DEFAULT_TYPES_FOR_VTK
  VTKm_USE_DEFAULT_TYPES_FOR_ASCENT
  )

if (VTKm_DEFAULT_TYPES_HEADER)
  set(VTK_M_HAS_DEFAULT_TYPES_HEADER TRUE)
  get_filename_component(VTKm_DEFAULT_TYPES_HEADER_FILENAME ${VTKm_DEFAULT_TYPES_HEADER} NAME_WE)
  set(VTKm_DEFAULT_TYPES_HEADER_FILENAME "${VTKm_DEFAULT_TYPES_HEADER_FILENAME}.h")
  configure_file(${VTKm_DEFAULT_TYPES_HEADER}
    ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/internal/${VTKm_DEFAULT_TYPES_HEADER_FILENAME}
    @ONLY
    )
  vtkm_install_headers(vtkm/cont/internal
    ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/internal/${VTKm_DEFAULT_TYPES_HEADER_FILENAME}
    )
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DefaultTypes.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/DefaultTypes.h
  @ONLY
  )
vtkm_install_headers(vtkm/cont
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/DefaultTypes.h
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
add_subdirectory(kokkos)

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
if(TARGET vtkm::kokkos)
  list(APPEND backends vtkm::kokkos)
endif()

target_link_libraries(vtkm_cont PUBLIC vtkm_compiler_flags ${backends})
target_link_libraries(vtkm_cont PUBLIC Threads::Threads)
target_link_libraries(vtkm_cont PUBLIC vtkm_optionparser vtkm_diy vtkm_lcl)
if(TARGET vtkm_loguru)
  target_link_libraries(vtkm_cont PRIVATE vtkm_loguru)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(testing)
