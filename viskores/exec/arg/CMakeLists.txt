##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  AspectTagDefault.h
  BasicArg.h
  Boundary.h
  CellShape.h
  ExecutionSignatureTagBase.h
  Fetch.h
  FetchExtrude.h
  FetchTagArrayDirectIn.h
  FetchTagArrayDirectInOut.h
  FetchTagArrayDirectOut.h
  FetchTagArrayNeighborhoodIn.h
  FetchTagArrayTopologyMapIn.h
  FetchTagExecObject.h
  FetchTagCellSetIn.h
  FetchTagKeysIn.h
  FetchTagWholeCellSetIn.h
  IncidentElementCount.h
  IncidentElementIndices.h
  InputIndex.h
  OutputIndex.h
  ThreadIndices.h
  ThreadIndicesBasic.h
  ThreadIndicesBasic3D.h
  ThreadIndicesExtrude.h
  ThreadIndicesNeighborhood.h
  ThreadIndicesCellNeighborhood.h
  ThreadIndicesPointNeighborhood.h
  ThreadIndicesReduceByKey.h
  ThreadIndicesTopologyMap.h
  ValueCount.h
  VisitIndex.h
  WorkIndex.h
  )

viskores_declare_headers(${headers})
