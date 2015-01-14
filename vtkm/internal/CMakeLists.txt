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

set(headers
  ConfigureFor32.h
  ConfigureFor64.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  Invocation.h
  ListTagDetail.h
  )

vtkm_declare_headers(${headers})

vtkm_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
vtkm_pyexpander_generated_file(FunctionInterfaceDetailPost.h)
vtkm_pyexpander_generated_file(ListTagDetail.h)

add_subdirectory(testing)

