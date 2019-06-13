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
  AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h
  AugmentMergeTrees_SetAugmentedMergeArcs.h
  CompressTrees_Step.h
  ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild.h
  ComputeHyperAndSuperStructure_PermuteArcs.h
  ComputeHyperAndSuperStructure_ResetHyperparentsId.h
  ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs.h
  ComputeRegularStructure_LocateSuperarcs.h
  ComputeRegularStructure_SetArcs.h
  ContourTreeSuperNodeComparator.h
  ContourTreeNodeComparator.h
  FindDegrees_SubtractLHE.h
  FindDegrees_FindRHE.h
  FindDegrees_ResetUpAndDowndegree.h
  TransferLeafChains_CollapsePastRegular.h
  TransferLeafChains_InitInAndOutbound.h
  TransferLeafChains_TransferToContourTree.h
  WasNotTransferred.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
