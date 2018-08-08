##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2018 UT-Battelle, LLC.
##  Copyright 2018 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(unit_tests
  UnitTestOpenMPArrayHandle.cxx
  UnitTestOpenMPArrayHandleFancy.cxx
  UnitTestOpenMPArrayHandleVirtualCoordinates.cxx
  UnitTestOpenMPCellLocatorTwoLevelUniformGrid.cxx
  UnitTestOpenMPColorTable.cxx
  UnitTestOpenMPComputeRange.cxx
  UnitTestOpenMPDataSetExplicit.cxx
  UnitTestOpenMPDataSetSingleType.cxx
  UnitTestOpenMPDeviceAdapter.cxx
  UnitTestOpenMPImplicitFunction.cxx
  UnitTestOpenMPPointLocatorUniformGrid.cxx
  UnitTestOpenMPVirtualObjectHandle.cxx
  )
vtkm_unit_tests(OpenMP SOURCES ${unit_tests})

if (VTKm_ENABLE_TESTING)
  #We need to have all OpenMP tests run serially as they
  #will uses all the system cores, and we will cause a N*N thread
  #explosion which causes the tests to run slower than when run
  #serially
  foreach (test ${unit_tests})
    get_filename_component(tname ${test} NAME_WE)
    set_tests_properties(${tname} PROPERTIES
        RUN_SERIAL True
    )
  endforeach()
endif()
