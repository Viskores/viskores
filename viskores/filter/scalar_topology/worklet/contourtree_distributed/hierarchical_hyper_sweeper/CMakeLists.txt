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
  InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet.h
  InitializeIntrinsicVertexCountInitalizeCountsWorklet.h
  InitializeIntrinsicVertexCountSubtractLowEndWorklet.h
  ComputeSuperarcDependentWeightsWorklet.h
  ComputeSuperarcTransferWeightsWorklet.h
  TransferTargetComperator.h
  TransferWeightsUpdateRHEWorklet.h
  TransferWeightsUpdateLHEWorklet.h
  TransferWeightsUpdateRHEWorkletRound2.h
  TransferWeightsUpdateLHEWorkletRound2.h
)

viskores_declare_headers(${headers})
