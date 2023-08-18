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
  using ParticleType = typename DSIType::PType;

  ParticleAdvector(const vtkm::filter::flow::internal::BoundsMap& bm,
                   const std::vector<DSIType>& blocks,
                   const bool& useThreaded,
                   const bool& useAsyncComm)
    : Blocks(blocks)
    , BoundsMap(bm)
    , UseThreadedAlgorithm(useThreaded)
    , UseAsynchronousCommunication(useAsyncComm)
  {
  }

  vtkm::cont::PartitionedDataSet Execute(const vtkm::cont::ArrayHandle<ParticleType>& seeds,
                                         vtkm::FloatDefault stepSize)
  {
    if (!this->UseThreadedAlgorithm)
    {
      using AlgorithmType = vtkm::filter::flow::internal::AdvectAlgorithm<DSIType>;
      return this->RunAlgo<AlgorithmType>(seeds, stepSize);
    }
    else
    {
      using AlgorithmType = vtkm::filter::flow::internal::AdvectAlgorithmThreaded<DSIType>;
      return this->RunAlgo<AlgorithmType>(seeds, stepSize);
    }
  }

private:
  template <typename AlgorithmType>
  vtkm::cont::PartitionedDataSet RunAlgo(const vtkm::cont::ArrayHandle<ParticleType>& seeds,
                                         vtkm::FloatDefault stepSize)
  {
    AlgorithmType algo(this->BoundsMap, this->Blocks, this->UseAsynchronousCommunication);
    algo.Execute(seeds, stepSize);
    return algo.GetOutput();
  }

  std::vector<DSIType> Blocks;
  vtkm::filter::flow::internal::BoundsMap BoundsMap;
  bool UseThreadedAlgorithm;
  bool UseAsynchronousCommunication = true;
};

}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_ParticleAdvector_h
