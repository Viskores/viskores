##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  AdvectAlgorithm.h
  AdvectAlgorithmTerminator.h
  AdvectAlgorithmThreaded.h
  BoundsMap.h
  DataSetIntegrator.h
  DataSetIntegratorSteadyState.h
  DataSetIntegratorUnsteadyState.h
  FilterParticleAdvectionSteadyStateImpl.h
  FilterParticleAdvectionUnsteadyStateImpl.h
  GridMetaData.h
  LagrangianStructureHelpers.h
  ParticleAdvector.h
  ParticleExchanger.h
  )

# Note: The C++ source files are added to the flow library
# in the CMakeLists.txt in the parent directory.

viskores_declare_headers(${headers})
