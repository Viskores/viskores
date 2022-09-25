//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_PathParticle_h
#define vtk_m_filter_flow_PathParticle_h

#include <vtkm/filter/flow/FilterParticleAdvectionUnsteadyState.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

#include <vtkm/filter/flow/worklet/Analysis.h>
#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/Termination.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

class PathParticle;

template <>
struct FlowTraits<PathParticle>
{
  using ParticleType = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType = vtkm::worklet::flow::NoAnalysis<ParticleType>;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
};

/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class VTKM_FILTER_FLOW_EXPORT PathParticle
  : public vtkm::filter::flow::FilterParticleAdvectionUnsteadyState<PathParticle>
{
public:
  using ParticleType = typename FlowTraits<PathParticle>::ParticleType;
  using TerminationType = typename FlowTraits<PathParticle>::TerminationType;
  using AnalysisType = typename FlowTraits<PathParticle>::AnalysisType;
  using ArrayType = typename FlowTraits<PathParticle>::ArrayType;
  using FieldType = typename FlowTraits<PathParticle>::FieldType;

  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const;

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const;

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_PathParticle_h
