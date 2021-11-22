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

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/Pathline.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionAlgorithm.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Pathline::Pathline()
  : vtkm::filter::FilterTemporalParticleAdvection<Pathline>()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Pathline::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using AlgorithmType = vtkm::filter::particleadvection::PathlineAlgorithm;
  using ThreadedAlgorithmType = vtkm::filter::particleadvection::PathlineThreadedAlgorithm;

  this->ValidateOptions(input);

  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

  if (this->GetUseThreadedAlgorithm())
    return vtkm::filter::particleadvection::RunAlgo<DSIType, ThreadedAlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  else
    return vtkm::filter::particleadvection::RunAlgo<DSIType, AlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
} // namespace vtkm::filter
#endif
