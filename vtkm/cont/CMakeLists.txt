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

include_directories(${Boost_INCLUDE_DIRS})

set(headers
  ArrayHandle.h
  ArrayHandleCast.h
  ArrayHandleCartesianProduct.h
  ArrayHandleCompositeVector.h
  ArrayHandleConstant.h
  ArrayHandleCounting.h
  ArrayHandleGroupVec.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandlePermutation.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  Assert.h
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
  DataSetBuilderRegular.h
  DataSetFieldAdd.h
  DeviceAdapter.h
  DeviceAdapterAlgorithm.h
  DeviceAdapterSerial.h
  DynamicArrayHandle.h
  DynamicCellSet.h
  Error.h
  ErrorControl.h
  ErrorControlAssert.h
  ErrorControlBadAllocation.h
  ErrorControlBadType.h
  ErrorControlBadValue.h
  ErrorControlInternal.h
  ErrorExecution.h
  Field.h
  LogicalStructure.h
  RuntimeDeviceInformation.h
  Storage.h
  StorageBasic.h
  StorageImplicit.h
  StorageListTag.h
  Timer.h
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

vtkm_declare_headers(${impl_headers} ${headers})

add_subdirectory(cuda)
add_subdirectory(tbb)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
