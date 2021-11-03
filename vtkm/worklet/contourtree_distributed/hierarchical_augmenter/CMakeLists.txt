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
  IsAttachementPointPredicate.h
  IsAscendingDecorator.h
  IsAttachementPointNeededPredicate.h
  AttachmentSuperparentAndIndexComparator.h
  AttachmentIdsEqualComparator.h
  SetFirstAttachmentPointInRoundWorklet.h
  UpdateHyperstructureSetHyperarcsAndNodesWorklet.h
  UpdateHyperstructureSetSuperchildrenWorklet.h
  FindSuperparentForNecessaryNodesWorklet.h
  CopyBaseRegularStructureWorklet.h
  SetSuperparentSetDecorator.h
  AttachmentAndSupernodeComparator.h
  ResizeArraysBuildNewSupernodeIdsWorklet.h
  CreateSuperarcsWorklet.h
  HierarchicalAugmenterInOutData.h
)

vtkm_declare_headers(${headers})
