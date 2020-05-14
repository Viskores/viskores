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
  BOVDataSetReader.h
  ErrorIO.h
  DecodePNG.h
  EncodePNG.h
  ImageReader.h
  ImageWriter.h
  PixelTypes.h
  VTKDataSetReader.h
  VTKDataSetReaderBase.h
  VTKPolyDataReader.h
  VTKRectilinearGridReader.h
  VTKStructuredGridReader.h
  VTKStructuredPointsReader.h
  VTKUnstructuredGridReader.h
  VTKDataSetWriter.h
)

set(template_sources
  ImageReader.hxx
  ImageWriter.hxx
  PixelTypes.hxx
)

set(sources
  DecodePNG.cxx
  EncodePNG.cxx
)

vtkm_declare_headers(
  ${headers}
  ${template_sources}
)

vtkm_library(
  NAME vtkm_io
  SOURCES ${sources}
  HEADERS ${headers}
  TEMPLATE_SOURCES ${template_sources}
)

target_link_libraries(vtkm_io PUBLIC vtkm_cont PRIVATE vtkm_lodepng)

add_subdirectory(internal)
add_subdirectory(reader)
add_subdirectory(writer)
add_subdirectory(testing)
