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
  UnitTestPixelTypes.cxx
  UnitTestVTKDataSetReader.cxx
  UnitTestVTKDataSetWriter.cxx
)

vtkm_unit_tests(SOURCES ${unit_tests} ALL_BACKENDS LIBRARIES vtkm_lodepng)

if(NOT VTKm_ENABLE_RENDERING)
  return()
endif()

set(image_unit_tests
  UnitTestImageWriter.cxx
)

vtkm_unit_tests(NAME UnitTests_vtkm_io_image_testing SOURCES ${image_unit_tests} ALL_BACKENDS LIBRARIES vtkm_rendering vtkm_lodepng)
