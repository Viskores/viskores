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
##  Copyright 2014. Los Alamos National Security
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
  ArrayContainerControl.h
  ArrayContainerControlBasic.h
  ArrayContainerControlImplicit.h
  ArrayHandle.h
  ArrayHandleCompositeVector.h
  ArrayHandleCounting.h
  ArrayHandleUniformPointCoordinates.h
  ArrayPortal.h
  Assert.h
  ContainerListTag.h
  DeviceAdapter.h
  DeviceAdapterSerial.h
  DynamicArrayHandle.h
  DynamicPointCoordinates.h
  Error.h
  ErrorControl.h
  ErrorControlAssert.h
  ErrorControlBadValue.h
  ErrorControlInternal.h
  ErrorControlOutOfMemory.h
  ErrorExecution.h
  PointCoordinatesArray.h
  PointCoordinatesListTag.h
  PointCoordinatesUniform.h
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)

vtkm_declare_headers(${impl_headers} ${headers})


#-----------------------------------------------------------------------------
add_subdirectory(testing)
