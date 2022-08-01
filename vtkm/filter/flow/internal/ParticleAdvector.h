//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_ParticleAdvector_h
#define vtk_m_filter_flow_internal_ParticleAdvector_h

#include <vtkm/filter/flow/internal/AdvectAlgorithm.h>
#include <vtkm/filter/flow/internal/AdvectAlgorithmThreaded.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename DSIType>
class ParticleAdvector
{
public:
  ParticleAdvector(const vtkm::filter::flow::internal::BoundsMap& bm,
                   const std::vector<DSIType>& blocks,
                   const bool& useThreaded,
                   const vtkm::filter::flow::FlowResultType& parType)
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
    using ParticleTypeList = vtkm::List<vtkm::Particle, vtkm::ChargedParticle>;

    vtkm::cont::PartitionedDataSet result;
    seeds.CastAndCallForTypes<ParticleTypeList, VTKM_DEFAULT_STORAGE_LIST>(
      [&](const auto& concreteSeeds) {
        result = this->Execute(numSteps, stepSize, concreteSeeds);
      });

    return result;
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
      if (this->ResultType == vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE)
      {
        using AlgorithmType = vtkm::filter::flow::internal::
          AdvectAlgorithm<DSIType, vtkm::worklet::flow::ParticleAdvectionResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
      else
      {
        using AlgorithmType = vtkm::filter::flow::internal::
          AdvectAlgorithm<DSIType, vtkm::worklet::flow::StreamlineResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
    }
    else
    {
      if (this->ResultType == vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE)
      {
        using AlgorithmType = vtkm::filter::flow::internal::AdvectAlgorithmThreaded<
          DSIType,
          vtkm::worklet::flow::ParticleAdvectionResult,
          ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
      else
      {
        using AlgorithmType = vtkm::filter::flow::internal::
          AdvectAlgorithmThreaded<DSIType, vtkm::worklet::flow::StreamlineResult, ParticleType>;

        return this->RunAlgo<AlgorithmType, ParticleType>(numSteps, stepSize, seeds);
      }
    }
  }


  std::vector<DSIType> Blocks;
  vtkm::filter::flow::internal::BoundsMap BoundsMap;
  FlowResultType ResultType;
  bool UseThreadedAlgorithm;
};

}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_ParticleAdvector_h
