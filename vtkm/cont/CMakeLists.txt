##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayHandle.h
  ArrayHandleCast.h
  ArrayHandleCartesianProduct.h
  ArrayHandleCompositeVector.h
  ArrayHandleConstant.h
  ArrayHandleCounting.h
  ArrayHandleDiscard.h
  ArrayHandleGroupVec.h
  ArrayHandleGroupVecVariable.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandlePermutation.h
  ArrayHandleStreaming.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayHandleConcatenate.h
  ArrayRangeCompute.h
  ArrayRangeCompute.hxx
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
  CellSetStructured.hxx
  ImplicitFunction.hxx
  StorageBasic.hxx
  )

set(sources
  ArrayHandle.cxx
  CellSet.cxx
  CellSetStructured.cxx
  CoordinateSystem.cxx
  DynamicArrayHandle.cxx
  Field.cxx
  ImplicitFunction.cxx
  internal/SimplePolymorphicContainer.cxx
  StorageBasic.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayRangeCompute.cxx
  RuntimeDeviceTracker.cxx
  )

vtkm_library(
  SOURCES ${sources}
  WRAP_FOR_CUDA ${device_sources}
  HEADERS ${header_impls})
target_include_directories(vtkm_cont PRIVATE ${VTKm_BACKEND_INCLUDE_DIRS})

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

vtkm_declare_headers(${headers})

add_subdirectory(serial)
add_subdirectory(cuda)
add_subdirectory(tbb)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
