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
  UnitTestControlSignatureTag.cxx
  UnitTestTransportArrayIn.cxx
  UnitTestTransportArrayInOut.cxx
  UnitTestTransportArrayOut.cxx
  UnitTestTransportCellSetIn.cxx
  UnitTestTransportExecObject.cxx
  UnitTestTransportWholeArray.cxx
  UnitTestTypeCheckArray.cxx
  UnitTestTypeCheckCellSet.cxx
  UnitTestTypeCheckExecObject.cxx
  UnitTestTypeCheckKeys.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
