//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Pathline_hxx
#define vtk_m_filter_Pathline_hxx

#include <vtkm/filter/Pathline.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/StreamlineAlgorithm.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Pathline::Pathline()
  : vtkm::filter::FilterDataSetWithField<Pathline>()
  , UseThreadedAlgorithm(false)
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void Pathline::SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Pathline::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (this->NextDataSet.GetNumberOfPartitions() != input.GetNumberOfPartitions())
    throw vtkm::cont::ErrorFilterExecution("Number of partitions do not match");
  if (!(this->PreviousTime < this->NextTime))
    throw vtkm::cont::ErrorFilterExecution("Previous time must be less than Next time.");

  std::string activeField = this->GetActiveFieldName();
  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  using DSIType = vtkm::filter::particleadvection::TemporalDataSetIntegrator;
  std::vector<DSIType> dsi;

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto dsPrev = input.GetPartition(i);
    auto dsNext = this->NextDataSet.GetPartition(i);
    if (!dsPrev.HasPointField(activeField) || !dsNext.HasPointField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    dsi.push_back(
      DSIType(dsPrev, this->PreviousTime, dsNext, this->NextTime, blockId, activeField));
  }

  using AlgorithmType = vtkm::filter::particleadvection::PathlineAlgorithm;
  using ThreadedAlgorithmType = vtkm::filter::particleadvection::PathlineThreadedAlgorithm;

  if (this->GetUseThreadedAlgorithm())
    return vtkm::filter::particleadvection::RunAlgo<DSIType, ThreadedAlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  else
    return vtkm::filter::particleadvection::RunAlgo<DSIType, AlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool Pathline::MapFieldOntoOutput(vtkm::cont::DataSet&,
                                                   const vtkm::cont::Field&,
                                                   vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
