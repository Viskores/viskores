//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PathParticle_hxx
#define vtk_m_filter_PathParticle_hxx

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/PathParticle.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionAlgorithm.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename ParticleType>
inline VTKM_CONT PathParticleBase<ParticleType>::PathParticleBase()
  : vtkm::filter::FilterTemporalParticleAdvection<PathParticleBase<ParticleType>, ParticleType>()
{
}

//-----------------------------------------------------------------------------
template <typename ParticleType>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet PathParticleBase<ParticleType>::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using AlgorithmType = vtkm::filter::particleadvection::PathParticleAlgorithm;
  using ThreadedAlgorithmType = vtkm::filter::particleadvection::PathParticleThreadedAlgorithm;
  using TDSIType = vtkm::filter::particleadvection::TemporalDataSetIntegrator;

  this->ValidateOptions(input);

  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

  if (this->GetUseThreadedAlgorithm())
    return vtkm::filter::particleadvection::RunAlgo<TDSIType, ThreadedAlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  else
    return vtkm::filter::particleadvection::RunAlgo<TDSIType, AlgorithmType>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
} // namespace vtkm::filter
#endif
