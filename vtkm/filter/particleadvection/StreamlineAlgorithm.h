//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_StreamlineAlgorithm_h
#define vtk_m_filter_particleadvection_StreamlineAlgorithm_h

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


//pathline
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

#endif //vtk_m_filter_particleadvection_StreamlineAlgorithm_h
