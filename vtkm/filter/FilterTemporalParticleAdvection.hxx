//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_FilterTemporalParticleAdvection_hxx
#define vtk_m_filter_FilterTemporalParticleAdvection_hxx

#include <vtkm/filter/FilterTemporalParticleAdvection.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename Derived, typename ParticleType>
inline VTKM_CONT
FilterTemporalParticleAdvection<Derived, ParticleType>::FilterTemporalParticleAdvection()
  : vtkm::filter::FilterParticleAdvection<Derived, ParticleType>()
  , PreviousTime(0)
  , NextTime(0)
{
}

template <typename Derived, typename ParticleType>
void FilterTemporalParticleAdvection<Derived, ParticleType>::ValidateOptions(
  const vtkm::cont::PartitionedDataSet& input) const
{
  this->vtkm::filter::FilterParticleAdvection<Derived, ParticleType>::ValidateOptions();

  if (this->NextDataSet.GetNumberOfPartitions() != input.GetNumberOfPartitions())
    throw vtkm::cont::ErrorFilterExecution("Number of partitions do not match");
  if (!(this->PreviousTime < this->NextTime))
    throw vtkm::cont::ErrorFilterExecution("Previous time must be less than Next time.");
}

template <typename Derived, typename ParticleType>
std::vector<vtkm::filter::particleadvection::TemporalDataSetIntegrator>
FilterTemporalParticleAdvection<Derived, ParticleType>::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
{
  using DSIType = vtkm::filter::particleadvection::TemporalDataSetIntegrator;
  std::vector<DSIType> dsi;
  std::string activeField = this->GetActiveFieldName();

  if (boundsMap.GetTotalNumBlocks() == 0)
    throw vtkm::cont::ErrorFilterExecution("No input datasets.");

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto dsPrev = input.GetPartition(i);
    auto dsNext = this->NextDataSet.GetPartition(i);
    if (!(dsPrev.HasPointField(activeField) || dsPrev.HasCellField(activeField)) ||
        !(dsNext.HasPointField(activeField) || dsNext.HasCellField(activeField)))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    dsi.push_back(
      DSIType(dsPrev, this->PreviousTime, dsNext, this->NextTime, blockId, activeField));
  }

  return dsi;
}

//-----------------------------------------------------------------------------
template <typename Derived, typename ParticleType>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet
FilterTemporalParticleAdvection<Derived, ParticleType>::PrepareForExecution(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  return (static_cast<Derived*>(this))->DoExecute(input, policy);
}

}
} // namespace vtkm::filter
#endif
