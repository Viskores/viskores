//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_ParticleAdvector_h
#define vtk_m_filter_particleadvection_ParticleAdvector_h

#include <vtkm/filter/particleadvection/AdvectAlgorithm.h>
#include <vtkm/filter/particleadvection/AdvectAlgorithmThreaded.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename DSIType>
class ParticleAdvector
{
public:
  ParticleAdvector(const vtkm::filter::particleadvection::BoundsMap& bm,
                   const std::vector<DSIType*> blocks,
                   const bool& useThreaded,
                   const vtkm::filter::particleadvection::ParticleAdvectionResultType& parType)
    : Blocks(blocks)
    , BoundsMap(bm)
    , ResultType(parType)
    , UseThreadedAlgorithm(useThreaded)
  {
  }

  vtkm::cont::PartitionedDataSet Execute(vtkm::Id numSteps,
                                         vtkm::FloatDefault stepSize,
                                         const vtkm::cont::UnknownArrayHandle& seeds)
  {
    using ParticleArray = vtkm::cont::ArrayHandle<vtkm::Particle>;
    using ChargedParticleArray = vtkm::cont::ArrayHandle<vtkm::ChargedParticle>;

    if (seeds.IsBaseComponentType<vtkm::Particle>())
      return this->Execute(numSteps, stepSize, seeds.AsArrayHandle<ParticleArray>());
    else if (seeds.IsBaseComponentType<vtkm::ChargedParticle>())
      return this->Execute(numSteps, stepSize, seeds.AsArrayHandle<ChargedParticleArray>());

    throw vtkm::cont::ErrorFilterExecution("Unsupported options in AdvectAlgorithm");
  }

private:
  template <typename AlgorithmType, typename ParticleType>
  vtkm::cont::PartitionedDataSet RunAlgo(vtkm::Id numSteps,
                                         vtkm::FloatDefault stepSize,
                                         const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    AlgorithmType algo(this->BoundsMap, this->Blocks);
    algo.Execute(numSteps, stepSize, seeds);
    return algo.GetOutput();
  }

  template <typename ParticleType>
  vtkm::cont::PartitionedDataSet Execute(vtkm::Id numSteps,
                                         vtkm::FloatDefault stepSize,
                                         const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    if (!this->UseThreadedAlgorithm)
    {
      if (this->ResultType ==
          vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
      {
        using AlgorithmType = vtkm::filter::particleadvection::
          AdvectAlgorithm<DSIType, vtkm::worklet::ParticleAdvectionResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
      else
      {
        using AlgorithmType = vtkm::filter::particleadvection::
          AdvectAlgorithm<DSIType, vtkm::worklet::StreamlineResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
    }
    else
    {
      if (this->ResultType ==
          vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
      {
        using AlgorithmType = vtkm::filter::particleadvection::
          AdvectAlgorithmThreaded<DSIType, vtkm::worklet::ParticleAdvectionResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
      else
      {
        using AlgorithmType = vtkm::filter::particleadvection::
          AdvectAlgorithmThreaded<DSIType, vtkm::worklet::StreamlineResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
    }
  }


  std::vector<DSIType*> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  ParticleAdvectionResultType ResultType;
  bool UseThreadedAlgorithm;
};

}
}
}


#endif //vtk_m_filter_particleadvection_ParticleAdvector_h
