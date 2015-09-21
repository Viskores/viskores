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
  BinaryPredicates.h
  BinaryOperators.h
  CellShape.h
  CellTraits.h
  Extent.h
  ListTag.h
  Math.h
  Matrix.h
  Pair.h
  StaticAssert.h
  TopologyElementTag.h
  TypeListTag.h
  Types.h
  TypeTraits.h
  VectorAnalysis.h
  VecRectilinearPointCoordinates.h
  VecTraits.h
  VecVariable.h
  UnaryPredicates.h
  )

vtkm_pyexpander_generated_file(Math.h)

vtkm_declare_headers(${headers})

#-----------------------------------------------------------------------------
#first add all the components vtkm that are shared between control and exec
add_subdirectory(testing)
add_subdirectory(internal)

if(VTKm_ENABLE_OPENGL_INTEROP)
  add_subdirectory(opengl)
endif(VTKm_ENABLE_OPENGL_INTEROP)

#-----------------------------------------------------------------------------
#add the control and exec folders
add_subdirectory(cont)
add_subdirectory(exec)

#-----------------------------------------------------------------------------
#add the worklet folder
add_subdirectory(worklet)

#-----------------------------------------------------------------------------
#add the benchmarking folder
add_subdirectory(benchmarking)

