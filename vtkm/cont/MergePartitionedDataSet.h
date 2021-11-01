//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_MergePartitionedDataset_h
#define vtk_m_cont_MergePartitionedDataset_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

class DataSet;
class PartitionedDataSet;

//@{
/// \brief Functions to compute bounds for a single dataSset or partition dataset
///
/// These are utility functions that compute bounds for a single dataset or
/// partitioned dataset. When VTK-m is operating in an distributed environment,
/// these are bounds on the local process. To get global bounds across all
/// ranks, use `vtkm::cont::BoundsGlobalCompute` instead.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for PartitionedDataSet, partitions without the
/// chosen CoordinateSystem are skipped.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::DataSet MergePartitionedDataSet(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet);

//@}
}
}

#endif
