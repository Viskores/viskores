//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_flow_internal_ParticleAdvector_h
#define viskores_filter_flow_internal_ParticleAdvector_h

#include <viskores/filter/flow/internal/AdvectAlgorithm.h>
#include <viskores/filter/flow/internal/AdvectAlgorithmThreaded.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>

namespace viskores
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
  using ParticleType = typename DSIType::PType;

  ParticleAdvector(const viskores::filter::flow::internal::BoundsMap& bm,
                   const std::vector<DSIType>& blocks,
                   const bool& useThreaded,
                   const bool& useAsyncComm)
    : Blocks(blocks)
    , BoundsMap(bm)
    , UseThreadedAlgorithm(useThreaded)
    , UseAsynchronousCommunication(useAsyncComm)
  {
  }

  viskores::cont::PartitionedDataSet Execute(const viskores::cont::ArrayHandle<ParticleType>& seeds,
                                         viskores::FloatDefault stepSize)
  {
    if (!this->UseThreadedAlgorithm)
    {
      using AlgorithmType = viskores::filter::flow::internal::AdvectAlgorithm<DSIType>;
      return this->RunAlgo<AlgorithmType>(seeds, stepSize);
    }
    else
    {
      using AlgorithmType = viskores::filter::flow::internal::AdvectAlgorithmThreaded<DSIType>;
      return this->RunAlgo<AlgorithmType>(seeds, stepSize);
    }
  }

private:
  template <typename AlgorithmType>
  viskores::cont::PartitionedDataSet RunAlgo(const viskores::cont::ArrayHandle<ParticleType>& seeds,
                                         viskores::FloatDefault stepSize)
  {
    AlgorithmType algo(this->BoundsMap, this->Blocks, this->UseAsynchronousCommunication);
    algo.Execute(seeds, stepSize);
    return algo.GetOutput();
  }

  std::vector<DSIType> Blocks;
  viskores::filter::flow::internal::BoundsMap BoundsMap;
  bool UseThreadedAlgorithm;
  bool UseAsynchronousCommunication = true;
};

}
}
}
} //viskores::filter::flow::internal


#endif //viskores_filter_flow_internal_ParticleAdvector_h
