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
  ArrayManagerExecutionTBB.h
  DeviceAdapterAlgorithmTBB.h
  DeviceAdapterTagTBB.h
  )

if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.3)
  if("${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}" VERSION_LESS 4.0)
    message_error("VTK-m not tested with TBB before version 4.0. Please upgrade your TBB library.")
  endif()
  set(headers ${headers} parallel_sort.h)
endif()

vtkm_declare_headers(${headers})
