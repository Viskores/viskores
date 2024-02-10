//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_density_estimate_Statistics_h
#define vtk_m_filter_density_estimate_Statistics_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Computes descriptive statistics of an input field.
///
/// This filter computes the following statistics on the active field of the input.
///
/// - `N`
/// - `Min`
/// - `Max`
/// - `Sum`
/// - `Mean`
/// - `M2`
/// - `M3`
/// - `M4`
/// - `SampleStddev`
/// - `PopulationStddev`
/// - `SampleVariance`
/// - `PopulationVariance`
/// - `Skewness`
/// - `Kurtosis`
///
/// `M2`, `M3`, and `M4` are the second, third, and fourth moments, respectively.
///
/// Note that this filter treats the "sample" and the "population" as the same with the
/// same mean. The difference between the two forms of variance is how they are normalized.
/// The population variance is normalized by dividing the second moment by `N`. The sample
/// variance uses Bessel's correction and divides the second moment by `N`-1 instead.
/// The standard deviation, which is just the square root of the variance, follows the
/// same difference.
///
/// The result of this filter is stored in a `vtkm::cont::DataSet` with no points
/// or cells. It contains only fields with the same names as the list above.
/// All fields have an association of `vtkm::cont::Field::Association::WholeDataSet`.
///
/// If `Execute` is called with a `vtkm::cont::PartitionedDataSet`, then the partitions
/// of the output will match those of the input. Additionally, the containing
/// `vtkm::cont::PartitionedDataSet` will contain the same fields associated with
/// `vtkm::cont::Field::Association::Global` that provide the overall statistics of
/// all partitions.
///
/// If this filter is used inside of an MPI job, then each `vtkm::cont::DataSet` result
/// will be @e local to the MPI rank. If `Execute` is called with a
/// `vtkm::cont::PartitionedDataSet`, then the fields attached to the
/// `vtkm::cont::PartitionedDataSet` container will have the overall statistics across
/// all MPI ranks (in addition to all partitions). Global MPI statistics for a single
/// `vtkm::cont::DataSet` can be computed by creating a `vtkm::cont::PartitionedDataSet`
/// with that as a single partition.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT Statistics : public vtkm::filter::Filter
{
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_density_estimate_Statistics_h
