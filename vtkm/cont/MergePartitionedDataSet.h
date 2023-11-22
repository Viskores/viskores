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
/// \brief This function can merge multiple data sets into on data set.
/// This function assume all input partitions have the same coordinates systems.
/// If a field does not exist in a specific partition but exists in other partitions,
/// the invalide value will be used to fill the coresponding region of that field in the merged data set.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::DataSet MergePartitionedDataSet(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  vtkm::Float64 invalidValue = vtkm::Nan64());

//@}
}
}

#endif
