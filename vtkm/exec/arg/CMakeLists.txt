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
  FetchTagArrayDirectOutArrayHandleGroupVecVariable.h
  FetchTagArrayDirectOutArrayHandleRecombineVec.h
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

vtkm_declare_headers(${headers})


#-----------------------------------------------------------------------------
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
