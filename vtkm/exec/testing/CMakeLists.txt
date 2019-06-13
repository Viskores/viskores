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
  UnitTestCellDerivative.cxx
  UnitTestCellEdgeFace.cxx
  UnitTestCellInterpolate.cxx
  UnitTestParametricCoordinates.cxx
  )
vtkm_unit_tests(SOURCES ${unit_tests})
