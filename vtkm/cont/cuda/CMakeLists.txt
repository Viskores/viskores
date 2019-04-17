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
  ChooseCudaDevice.h
  DeviceAdapterCuda.h
  ErrorCuda.h
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
vtkm_declare_headers(${headers})

#-----------------------------------------------------------------------------
if (TARGET vtkm::cuda)
  add_subdirectory(testing)
endif()

