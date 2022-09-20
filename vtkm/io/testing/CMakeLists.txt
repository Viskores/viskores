##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(unit_tests
  UnitTestBOVDataSetReader.cxx
  UnitTestFileUtils.cxx
  UnitTestPixelTypes.cxx
  UnitTestVTKDataSetReader.cxx
  UnitTestVTKDataSetWriter.cxx
)

set(unit_test_libraries vtkm_io)

if(VTKm_ENABLE_RENDERING)
  list(APPEND unit_tests
    UnitTestImageWriter.cxx
  )

  if (VTKm_ENABLE_HDF5_IO)
    list(APPEND unit_tests
      UnitTestHDF5Image.cxx)
  endif()

  list(APPEND unit_test_libraries vtkm_rendering)
endif()

vtkm_unit_tests(SOURCES ${unit_tests} LIBRARIES ${unit_test_libraries})
