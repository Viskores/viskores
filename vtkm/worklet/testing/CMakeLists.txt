#============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(unit_tests
  UnitTestAverageByKey.cxx
  UnitTestCellAverage.cxx
  UnitTestCellDeepCopy.cxx
  UnitTestCellGradient.cxx
  UnitTestClipping.cxx
  UnitTestContourTreeUniform.cxx
  UnitTestCosmoTools.cxx
  UnitTestExternalFaces.cxx
  UnitTestExtractGeometry.cxx
  UnitTestExtractPoints.cxx
  UnitTestExtractStructured.cxx
  UnitTestFieldHistogram.cxx
  UnitTestFieldStatistics.cxx
  UnitTestKdTreeBuildNNS.cxx
  UnitTestKeys.cxx
  UnitTestMagnitude.cxx
  UnitTestMarchingCubes.cxx
  UnitTestMask.cxx
  UnitTestMaskPoints.cxx
  UnitTestNDimsEntropy.cxx
  UnitTestNDimsHistogram.cxx
  UnitTestNDimsHistMarginalization.cxx
  UnitTestParticleAdvection.cxx
  UnitTestPointElevation.cxx
  UnitTestPointGradient.cxx
  UnitTestRemoveUnusedPoints.cxx
  UnitTestScatterCounting.cxx
  UnitTestScatterPermutation.cxx
  UnitTestSplatKernels.cxx
  UnitTestStreamingSine.cxx
  UnitTestStreamLineUniformGrid.cxx
  UnitTestSurfaceNormals.cxx
  UnitTestTetrahedralize.cxx
  UnitTestThreshold.cxx
  UnitTestThresholdPoints.cxx
  UnitTestTriangulate.cxx
  UnitTestWholeCellSetIn.cxx
  UnitTestWorkletMapField.cxx
  UnitTestWorkletMapFieldExecArg.cxx
  UnitTestWorkletMapFieldWholeArray.cxx
  UnitTestWorkletMapPointNeighborhood.cxx
  UnitTestWorkletMapTopologyExplicit.cxx
  UnitTestWorkletMapTopologyUniform.cxx
  UnitTestWorkletReduceByKey.cxx
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
