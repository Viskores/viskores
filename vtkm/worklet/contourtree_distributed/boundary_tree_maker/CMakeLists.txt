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
  FindBoundaryVerticesIsNecessaryWorklet.h
  BoundaryVerticesPerSuperArcWorklets.h
  PropagateBoundaryCountsSubtractDependentCountsWorklet.h
  PropagateBoundaryCountsTransferDependentCountsWorklet.h
  PropagateBoundaryCountsTransferCumulativeCountsWorklet.h
  PropagateBoundaryCountsComputeGroupTotalsWorklet.h
  FindNecessaryInteriorSupernodesFindNodesWorklet.h
  FindNecessaryInteriorSetSuperparentNecessaryWorklet.h
  AugmentBoundaryWithNecessaryInteriorSupernodesUnsetBoundarySupernodesWorklet.h
  AugmentBoundaryWithNecessaryInteriorSupernodesAppendNecessarySupernodesWorklet.h
  FindBoundaryTreeSuperarcsSuperarcToWorklet.h
  SetUpAndDownNeighboursWorklet.h
  IdentifyRegularisedSupernodesStepOneWorklet.h
  IdentifyRegularisedSupernodesStepTwoWorklet.h
  AddTerminalFlagsToUpDownNeighboursWorklet.h
  PointerDoubleUpDownNeighboursWorklet.h
  CompressRegularisedNodesCopyNecessaryRegularNodesWorklet.h
  CompressRegularisedNodesFindNewSuperarcsWorklet.h
  CompressRegularisedNodesResolveRootWorklet.h
  CompressRegularisedNodesTransferVerticesWorklet.h
  CompressRegularisedNodesFillBoundaryTreeSuperarcsWorklet.h
  SetInteriorForestWorklet.h
  SumFunctor.h
  NoSuchElementFunctor.h
  HyperarcComparator.h
  ContourTreeNodeHyperArcComparator.h
  BoundaryTreeNodeComparator.h
)

vtkm_declare_headers(${headers})
