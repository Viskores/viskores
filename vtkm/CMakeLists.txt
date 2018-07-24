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

# Configure version file. (Other configuration in internal/Configure.h)
vtkm_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
  ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Version.h
  @ONLY)
vtkm_install_headers(
  vtkm ${VTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Version.h)

set(headers
  Assert.h
  BaseComponent.h
  BinaryPredicates.h
  BinaryOperators.h
  Bounds.h
  CellShape.h
  CellTraits.h
  Flags.h
  Geometry.h
  Hash.h
  ImplicitFunction.h
  ListTag.h
  Math.h
  Matrix.h
  NewtonsMethod.h
  Pair.h
  Range.h
  RangeId.h
  RangeId3.h
  StaticAssert.h
  Swap.h
  TopologyElementTag.h
  Transform3D.h
  TypeListTag.h
  Types.h
  TypeTraits.h
  VecAxisAlignedPointCoordinates.h
  VecFromPortal.h
  VecFromPortalPermute.h
  VectorAnalysis.h
  VecTraits.h
  VecVariable.h
  VirtualObjectBase.h
  UnaryPredicates.h
)

set(template_sources
  Geometry.hxx
)

vtkm_pyexpander_generated_file(Math.h)

vtkm_declare_headers(
  ${headers}
  ${template_sources}
  EXCLUDE_FROM_TESTING
    ${template_sources}
)

#-----------------------------------------------------------------------------
#first add all the components vtkm that are shared between control and exec
add_subdirectory(thirdparty/diy)
add_subdirectory(thirdparty/taotuple)
add_subdirectory(testing)
add_subdirectory(internal)

#-----------------------------------------------------------------------------
#add the control and exec folders
add_subdirectory(cont)
add_subdirectory(exec)

#-----------------------------------------------------------------------------
#add the worklet folder
add_subdirectory(worklet)

#-----------------------------------------------------------------------------
#add the filter folder
add_subdirectory(filter)

#-----------------------------------------------------------------------------
# Build rendering
add_subdirectory(rendering)

add_subdirectory(interop)

#-----------------------------------------------------------------------------
#add the io folder
add_subdirectory(io)
