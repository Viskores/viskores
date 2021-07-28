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

using DSIType = vtkm::filter::particleadvection::DataSetIntegrator;
using TDSIType = vtkm::filter::particleadvection::TemporalDataSetIntegrator;

//-------------------------------------------------------------------------------------------
//Steady state advection algorithms

//Particle advection
class VTKM_ALWAYS_EXPORT ParticleAdvectionAlgorithm
  : public AdvectorBaseAlgorithm<DSIType, vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
public:
  ParticleAdvectionAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                             const std::vector<DSIType>& blocks)
    : AdvectorBaseAlgorithm<DSIType, vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm,
                                                                                             blocks)
  {
  }
};

//Threaded particle advection
class VTKM_ALWAYS_EXPORT ParticleAdvectionThreadedAlgorithm
  : public AdvectorBaseThreadedAlgorithm<DSIType,
                                         vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
public:
  ParticleAdvectionThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                     const std::vector<DSIType>& blocks)
    : AdvectorBaseThreadedAlgorithm<DSIType,
                                    vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm,
                                                                                            blocks)
  {
  }
};

//Streamline
class VTKM_ALWAYS_EXPORT StreamlineAlgorithm
  : public AdvectorBaseAlgorithm<DSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
public:
  StreamlineAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                      const std::vector<DSIType>& blocks)
    : AdvectorBaseAlgorithm<DSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>(bm, blocks)
  {
  }
};

//Threaded streamline

class VTKM_ALWAYS_EXPORT StreamlineThreadedAlgorithm
  : public AdvectorBaseThreadedAlgorithm<DSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
public:
  StreamlineThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                              const std::vector<DSIType>& blocks)
    : AdvectorBaseThreadedAlgorithm<DSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>(
        bm,
        blocks)
  {
  }
};

//-------------------------------------------------------------------------------------------
//Unsteady state advection algorithms

//PathParticle
class VTKM_ALWAYS_EXPORT PathParticleAlgorithm
  : public AdvectorBaseAlgorithm<TDSIType, vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
public:
  PathParticleAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                        const std::vector<TDSIType>& blocks)
    : AdvectorBaseAlgorithm<TDSIType, vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(
        bm,
        blocks)
  {
  }
};


//Threaded PathParticle
class VTKM_ALWAYS_EXPORT PathParticleThreadedAlgorithm
  : public AdvectorBaseThreadedAlgorithm<TDSIType,
                                         vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
public:
  PathParticleThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                const std::vector<TDSIType>& blocks)
    : AdvectorBaseThreadedAlgorithm<TDSIType,
                                    vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm,
                                                                                            blocks)
  {
  }
};

//Pathline
class VTKM_ALWAYS_EXPORT PathlineAlgorithm
  : public AdvectorBaseAlgorithm<TDSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
public:
  PathlineAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                    const std::vector<TDSIType>& blocks)
    : AdvectorBaseAlgorithm<TDSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>(bm, blocks)
  {
  }
};

//Threaded pathline
class VTKM_ALWAYS_EXPORT PathlineThreadedAlgorithm
  : public AdvectorBaseThreadedAlgorithm<TDSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
public:
  PathlineThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                            const std::vector<TDSIType>& blocks)
    : AdvectorBaseThreadedAlgorithm<TDSIType, vtkm::worklet::StreamlineResult<vtkm::Particle>>(
        bm,
        blocks)
  {
  }
};



}
}
} // namespace vtkm::filter::particleadvection

#endif //vtk_m_filter_particleadvection_ParticleAdvectionAlgorithm_h
