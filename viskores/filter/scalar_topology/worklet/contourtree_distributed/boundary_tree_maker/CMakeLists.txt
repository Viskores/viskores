##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  FindBoundaryVerticesIsNecessaryWorklet.h
  BoundaryVerticesPerSuperArcWorklets.h
  PropagateBoundaryCountsSubtractDependentCountsWorklet.h
  PropagateBoundaryCountsTransferDependentCountsWorklet.h
  PropagateBoundaryCountsTransferCumCountsWorklet.h
  PropagateBoundaryCountsComputeGroupTotalsWorklet.h
  FindNecessaryInteriorSupernodesFindNodesWorklet.h
  FindNecessaryInteriorSetSuperparentNecessaryWorklet.h
  AugmentBoundaryUnsetSupernodesWorklet.h
  AugmentBoundaryAppendSupernodesWorklet.h
  FindBoundaryTreeSuperarcsSuperarcToWorklet.h
  SetUpAndDownNeighboursWorklet.h
  IdentifyRegularisedSupernodesStepOneWorklet.h
  IdentifyRegularisedSupernodesStepTwoWorklet.h
  AddTerminalFlagsToUpDownNeighboursWorklet.h
  PointerDoubleUpDownNeighboursWorklet.h
  CompressRegularisedNodesCopyRegularNodesWorklet.h
  CompressRegularisedNodesFindNewSuperarcsWorklet.h
  CompressRegularisedNodesResolveRootWorklet.h
  CompressRegularisedNodesTransferVerticesWorklet.h
  CompressRegularisedNodesFillBTreeSuperarcsWorklet.h
  SetInteriorForestWorklet.h
  SumFunctor.h
  NoSuchElementFunctor.h
  HyperarcComparator.h
  ContourTreeNodeHyperArcComparator.h
  BoundaryTreeNodeComparator.h
)

viskores_declare_headers(${headers})
