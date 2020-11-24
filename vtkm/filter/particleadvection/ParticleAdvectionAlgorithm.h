//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_ParticleAdvectionAlgorithm_h
#define vtk_m_filter_particleadvection_ParticleAdvectionAlgorithm_h

#include <vtkm/filter/particleadvection/AdvectorBaseAlgorithm.h>
#include <vtkm/filter/particleadvection/AdvectorBaseThreadedAlgorithm.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class VTKM_ALWAYS_EXPORT ParticleAdvectionAlgorithm
  : public AdvectorBaseAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  ParticleAdvectionAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                             const std::vector<DataSetIntegratorType>& blocks)
    : AdvectorBaseAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm, blocks)
  {
  }
};

class VTKM_ALWAYS_EXPORT ParticleAdvectionThreadedAlgorithm
  : public AdvectorBaseThreadedAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  ParticleAdvectionThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                     const std::vector<DataSetIntegratorType>& blocks)
    : AdvectorBaseThreadedAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm,
                                                                                            blocks)
  {
  }
};

}
}
} // namespace vtkm::filter::particleadvection

#endif //vtk_m_filter_particleadvection_ParticleAdvectionAlgorithm_h
