//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_FieldRangeCompute_h
#define vtk_m_cont_FieldRangeCompute_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <numeric>

namespace vtkm
{
namespace cont
{
/// \brief Compute ranges for fields in a DataSet or PartitionedDataSet.
///
/// These methods to compute ranges for fields in a single dataset or a
/// partitioned dataset.
/// When using VTK-m in a hybrid-parallel environment with distributed processing,
/// this class uses ranges for locally available data alone. Use FieldRangeGlobalCompute
/// to compute ranges globally across all ranks even in distributed mode.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any);

template <typename TypeList>
VTKM_DEPRECATED(1.6, "FieldRangeCompute no longer supports TypeList.")
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  VTKM_IS_LIST(TypeList);
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

  VTKM_DEPRECATED_SUPPRESS_BEGIN
  return field.GetRange(TypeList());
  VTKM_DEPRECATED_SUPPRESS_END
}

//@}

//{@
/// Returns the range for a field from a PartitionedDataSet. If the field is
/// not present on any of the partitions, an empty ArrayHandle will be
/// returned. If the field is present on some partitions, but not all, those
/// partitions without the field are skipped.
///
/// The returned array handle will have as many values as the maximum number of
/// components for the selected field across all partitions.
///
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(
  const vtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any);

template <typename TypeList>
VTKM_DEPRECATED(1.6, "FieldRangeCompute no longer supports TypeList.")
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(
  const vtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  VTKM_IS_LIST(TypeList);
  VTKM_STATIC_ASSERT_MSG((!std::is_same<TypeList, vtkm::ListUniversal>::value),
                         "Cannot use vtkm::ListUniversal with FieldRangeCompute.");
  std::vector<vtkm::Range> result_vector = std::accumulate(
    pds.begin(),
    pds.end(),
    std::vector<vtkm::Range>(),
    [&](const std::vector<vtkm::Range>& accumulated_value, const vtkm::cont::DataSet& dataset) {
      VTKM_DEPRECATED_SUPPRESS_BEGIN
      vtkm::cont::ArrayHandle<vtkm::Range> partition_range =
        vtkm::cont::FieldRangeCompute(dataset, name, assoc, TypeList());
      VTKM_DEPRECATED_SUPPRESS_END

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

//@}
}
} // namespace vtkm::cont

#endif
