##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  CosmoTools.h
  CosmoToolsCenterFinder.h
  CosmoToolsHaloFinder.h
  ComputeBins.h
  ComputeBinIndices.h
  ComputeBinRange.h
  ComputeNeighborBins.h
  ComputePotential.h
  ComputePotentialBin.h
  ComputePotentialNeighbors.h
  ComputePotentialNxN.h
  ComputePotentialMxN.h
  ComputePotentialOnCandidates.h
  EqualsMinimumPotential.h
  GraftParticles.h
  IsStar.h
  MarkActiveNeighbors.h
  PointerJump.h
  SetCandidateParticles.h
  TagTypes.h
  ValidHalo.h
  )

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})
