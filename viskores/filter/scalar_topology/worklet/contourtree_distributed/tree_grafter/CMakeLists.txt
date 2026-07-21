##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  GetHierarchicalIdsWorklet.h
  InitActiceSuperarcIdWorklet.h
  InitActiceSuperarcsWorklet.h
  GraftInteriorForestsSetTransferIterationWorklet.h
  FindCriticalPointsSetUpDownNeighboursWorklet.h
  FindCriticalPointsFindSaddlesWorklet.h
  FindCriticalPointsFindLeafsWorklet.h
  FindCriticalPointsFindTerminalElementsWorklet.h
  CollapseRegularChainsWorklet.h
  IdentifyLeafHyperarcsWorklet.h
  SuperarcWasNotTransferredPredicate.h
  NewHypernodePredicate.h
  HyperNodeWhenComparator.h
  SuperNodeWhenComparator.h
  NewNodePredicate.h
  ListNewNodesCopyIdsWorklet.h
  CopyNewHypernodesWorklet.h
  CopyNewSupernodesWorklet.h
  CopyNewSupernodesSetSuperchildrenWorklet.h
  PermuteComparator.h
  CopyNewNodesSetSuperparentsWorklet.h
  CopyFirstSupernodePerIterationWorklet.h
  CopyFirstHypernodePerIterationWorklet.h
  CalculateAttachementCounterWorklet.h
)

viskores_declare_headers(${headers})
