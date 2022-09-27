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
  AdvectAlgorithm.h
  AdvectAlgorithmThreaded.h
  BoundsMap.h
  DataSetIntegrator.h
  DataSetIntegratorSteadyState.h
  DataSetIntegratorUnsteadyState.h
  GridMetaData.h
  LagrangianStructureHelpers.h
  Messenger.h
  ParticleAdvector.h
  ParticleMessenger.h
  )

# Note: The C++ source files are added to the flow library
# in the CMakeLists.txt in the parent directory.

vtkm_declare_headers(${headers})
