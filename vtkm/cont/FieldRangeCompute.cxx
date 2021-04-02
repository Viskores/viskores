//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/FieldRangeCompute.h>

#include <vtkm/cont/Algorithm.h>

namespace vtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(const vtkm::cont::DataSet& dataset,
                                                       const std::string& name,
                                                       vtkm::cont::Field::Association assoc)
{
  vtkm::cont::Field field;
  try
  {
    field = dataset.GetField(name, assoc);
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    // field missing, return empty range.
    return vtkm::cont::ArrayHandle<vtkm::Range>();
  }

  return field.GetRange();
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(const vtkm::cont::PartitionedDataSet& pds,
                                                       const std::string& name,
                                                       vtkm::cont::Field::Association assoc)
{
  std::vector<vtkm::Range> result_vector = std::accumulate(
    pds.begin(),
    pds.end(),
    std::vector<vtkm::Range>(),
    [&](const std::vector<vtkm::Range>& accumulated_value, const vtkm::cont::DataSet& dataset) {
      vtkm::cont::ArrayHandle<vtkm::Range> partition_range =
        vtkm::cont::FieldRangeCompute(dataset, name, assoc);

      std::vector<vtkm::Range> result = accumulated_value;

      // if the current partition has more components than we have seen so far,
      // resize the result to fit all components.
      result.resize(
        std::max(result.size(), static_cast<size_t>(partition_range.GetNumberOfValues())));

      auto portal = partition_range.ReadPortal();
      std::transform(vtkm::cont::ArrayPortalToIteratorBegin(portal),
                     vtkm::cont::ArrayPortalToIteratorEnd(portal),
                     result.begin(),
                     result.begin(),
                     std::plus<vtkm::Range>());
      return result;
    });

  return vtkm::cont::make_ArrayHandleMove(std::move(result_vector));
}
}
} // namespace vtkm::cont
