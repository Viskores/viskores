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
      const bool& useThreaded)
    : Blocks(blocks)
    , BoundsMap(bm)
    , UseThreadedAlgorithm(useThreaded)
  {
  }

  vtkm::cont::PartitionedDataSet Execute(vtkm::Id numSteps,
                                         vtkm::FloatDefault stepSize,
                                         const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
  {
    if (!this->UseThreadedAlgorithm)
    {
      using AlgorithmType =
        vtkm::filter::particleadvection::ABA<vtkm::worklet::ParticleAdvectionResult,
                                             vtkm::Particle>;

      AlgorithmType algo(this->BoundsMap, this->Blocks);
      algo.Execute(numSteps, stepSize, seeds);
      return algo.GetOutput();
    }
    else
    {
      //using ThreadedAlgorithmType = vtkm::filter::particleadvection::ParticleAdvectionThreadedAlgorithm;
      //AlgorithmType algo;
    }

    throw vtkm::cont::ErrorFilterExecution("Unsupported options in ABA");
  }

  std::vector<DSI> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  bool UseThreadedAlgorithm;
};

}
}
}


#endif //vtk_m_filter_particleadvection_PAV_h
