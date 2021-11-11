//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleAdvection_hxx
#define vtk_m_filter_ParticleAdvection_hxx

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionAlgorithm.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ParticleAdvection::ParticleAdvection()
  : vtkm::filter::FilterParticleAdvection<ParticleAdvection, vtkm::Particle>()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet ParticleAdvection::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using AlgorithmType = vtkm::filter::particleadvection::ParticleAdvectionAlgorithm;
  using AlgorithmType2 = vtkm::filter::particleadvection::ParticleAdvectionAlgorithm2;
  using ThreadedAlgorithmType = vtkm::filter::particleadvection::ParticleAdvectionThreadedAlgorithm;

  this->ValidateOptions();
  vtkm::filter::particleadvection::BoundsMap boundsMap(input);

  if (this->ActiveFieldName2.empty())
  {
    auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

    if (this->GetUseThreadedAlgorithm())
      return vtkm::filter::particleadvection::RunAlgo<DSIType, ThreadedAlgorithmType>(
        boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
    else
      return vtkm::filter::particleadvection::RunAlgo<DSIType, AlgorithmType>(
        boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  }
  else
  {
    std::vector<vtkm::filter::particleadvection::DataSetIntegrator2> dsi;
    std::string field1 = this->GetActiveFieldName();
    std::string field2 = this->ActiveFieldName2;

    for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
    {
      vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
      auto ds = input.GetPartition(i);
      if (!ds.HasPointField(field1) || !ds.HasPointField(field2))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
      dsi.push_back(DSIType2(ds, blockId, field1, field2));
    }

    std::cout << "Electromagnietc RunAlgo" << std::endl;
    return vtkm::filter::particleadvection::RunAlgo<DSIType2, AlgorithmType2>(
      boundsMap, dsi, this->NumberOfSteps, this->StepSize, this->Seeds);
  }
}

}
} // namespace vtkm::filter
#endif
