//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_FilterParticleAdvection_hxx
#define vtk_m_filter_FilterParticleAdvection_hxx

#include <vtkm/filter/FilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT FilterParticleAdvection<Derived>::FilterParticleAdvection()
  : vtkm::filter::FilterDataSetWithField<Derived>()
  , NumberOfSteps(0)
  , StepSize(0)
  , UseThreadedAlgorithm(false)
{
}

template <typename Derived>
void FilterParticleAdvection<Derived>::ValidateOptions() const
{
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (this->NumberOfSteps == 0)
    throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
}

template <typename Derived>
std::vector<vtkm::filter::particleadvection::DataSetIntegrator>
FilterParticleAdvection<Derived>::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
{
  std::vector<vtkm::filter::particleadvection::DataSetIntegrator> dsi;

  if (boundsMap.GetTotalNumBlocks() == 0)
    throw vtkm::cont::ErrorFilterExecution("No input datasets.");

  std::string activeField = this->GetActiveFieldName();

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (!ds.HasPointField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    dsi.push_back(DSIType(ds, blockId, activeField));
  }

  return dsi;
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet FilterParticleAdvection<Derived>::PrepareForExecution(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  return (static_cast<Derived*>(this))->DoExecute(input, policy);
}

}
} // namespace vtkm::filter
#endif
