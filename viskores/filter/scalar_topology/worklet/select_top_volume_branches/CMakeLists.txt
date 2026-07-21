##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  Predicates.h
  TopVolumeBranchData.h
  AboveThresholdWorklet.h
  AssignValueWorklet.h
  BinarySearchWorklet.h
  BranchSaddleIsKnownWorklet.h
  ClarifyBranchEndSupernodeTypeWorklet.h
  CollectOuterSaddleWorklet.h
  UpdateOuterSaddleWorklet.h
  UpdateInfoByBranchDirectionWorklet.h
  GetParentBranchWorklet.h
  GetBranchHierarchyWorklet.h
  GetBranchVolumeWorklet.h
  BranchParentComparator.h
  BranchVolumeComparator.h
  BranchDecompositionTreeMaker.h
)
#-----------------------------------------------------------------------------

viskores_declare_headers(${headers})
