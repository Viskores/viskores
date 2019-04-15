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
vtkm_declare_headers(${headers})
