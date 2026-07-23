##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  BranchEndComparator.h
  BranchEndGlobalUpdateWorklet.h
  CollapseBranchesPtrDoublingWorklet.h
  CollapseBranchesWorklet.h
  FindBestSupernodeWorklet.h
  GetOuterEndWorklet.h
  LocalBestUpDownVolBestEdgeWorklet.h
  LocalBestUpDownVolInitSuperarcList.h
  LocalBestUpDownByVolumeWorklet.h
  SuperArcVolComparatorGlobalId.h
)

viskores_declare_headers(${headers})
