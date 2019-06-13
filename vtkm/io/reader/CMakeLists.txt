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
  VTKDataSetReaderBase.h
  VTKPolyDataReader.h
  VTKStructuredGridReader.h
  VTKRectilinearGridReader.h
  VTKStructuredPointsReader.h
  VTKUnstructuredGridReader.h
  VTKDataSetReader.h
)

vtkm_declare_headers(${headers})

add_subdirectory(testing)
