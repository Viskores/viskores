#============================================================================
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

set(unit_tests
  UnitTestCellAverage.cxx
  UnitTestCellDeepCopy.cxx
  UnitTestCellGradient.cxx
  UnitTestClipping.cxx
  UnitTestExternalFaces.cxx
  UnitTestFieldHistogram.cxx
  UnitTestFieldStatistics.cxx
  UnitTestMarchingCubes.cxx
  UnitTestPointElevation.cxx
  UnitTestRemoveUnusedPoints.cxx
  UnitTestScatterCounting.cxx
  UnitTestSplatKernels.cxx
  UnitTestStreamingSine.cxx
  UnitTestStreamLineUniformGrid.cxx
  UnitTestTetrahedralizeExplicitGrid.cxx
  UnitTestTetrahedralizeUniformGrid.cxx
  UnitTestThreshold.cxx
  UnitTestWorkletMapField.cxx
  UnitTestWorkletMapFieldExecArg.cxx
  UnitTestWorkletMapFieldWholeArray.cxx
  UnitTestWorkletMapTopologyExplicit.cxx
  UnitTestWorkletMapTopologyUniform.cxx
  UnitTestVertexClustering.cxx
  UnitTestWaveletCompressor.cxx
 )

vtkm_save_worklet_unit_tests(${unit_tests})

vtkm_worklet_unit_tests( VTKM_DEVICE_ADAPTER_SERIAL )

if (VTKm_ENABLE_CUDA)
  vtkm_worklet_unit_tests( VTKM_DEVICE_ADAPTER_CUDA )
endif()
if (VTKm_ENABLE_TBB)
  vtkm_worklet_unit_tests( VTKM_DEVICE_ADAPTER_TBB )
endif()
