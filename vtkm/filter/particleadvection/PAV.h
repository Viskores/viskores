//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_PAV_h
#define vtk_m_filter_particleadvection_PAV_h

#include <vtkm/filter/particleadvection/ABA.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DSI.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class PAV
{
public:
  PAV(const vtkm::filter::particleadvection::BoundsMap& bm,
      const std::vector<DSI>& blocks,
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

    throw vtkm::cont::ErrorFilterExecution("Unsupported options in ABA");
  }

private:
  template <typename ParticleType>
  vtkm::cont::PartitionedDataSet Execute(vtkm::Id numSteps,
                                         vtkm::FloatDefault stepSize,
                                         const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    if (!this->UseThreadedAlgorithm)
    {
      //make a templated algorithm execution()
      if (this->ResultType ==
          vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
      {
        using AlgorithmType =
          vtkm::filter::particleadvection::ABA<vtkm::worklet::ParticleAdvectionResult,
                                               ParticleType>;

        AlgorithmType algo(this->BoundsMap, this->Blocks);
        algo.Execute(numSteps, stepSize, seeds);
        return algo.GetOutput();
      }
      else
      {
        using AlgorithmType =
          vtkm::filter::particleadvection::ABA<vtkm::worklet::StreamlineResult, ParticleType>;

        AlgorithmType algo(this->BoundsMap, this->Blocks);
        algo.Execute(numSteps, stepSize, seeds);
        return algo.GetOutput();
      }
    }
    else
    {
      std::cout << "Change me to threaded ABA" << std::endl;
      using AlgorithmType =
        vtkm::filter::particleadvection::ABA<vtkm::worklet::ParticleAdvectionResult, ParticleType>;

      AlgorithmType algo(this->BoundsMap, this->Blocks);
      algo.Execute(numSteps, stepSize, seeds);
      return algo.GetOutput();
    }
  }




  std::vector<DSI> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  ParticleAdvectionResultType ResultType;
  bool UseThreadedAlgorithm;
};

}
}
}


#endif //vtk_m_filter_particleadvection_PAV_h
