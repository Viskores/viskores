//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_WarpXStreamline_h
#define vtk_m_filter_flow_WarpXStreamline_h

#include <vtkm/filter/flow/FilterParticleAdvectionSteadyState.h>
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

class WarpXStreamline;

template <>
struct FlowTraits<WarpXStreamline>
{
  using ParticleType = vtkm::ChargedParticle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType = vtkm::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::ElectroMagneticField<ArrayType>;
};

/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.
class VTKM_FILTER_FLOW_EXPORT WarpXStreamline
  : public vtkm::filter::flow::FilterParticleAdvectionSteadyState<WarpXStreamline>
{
public:
  using ParticleType = typename FlowTraits<WarpXStreamline>::ParticleType;
  using TerminationType = typename FlowTraits<WarpXStreamline>::TerminationType;
  using AnalysisType = typename FlowTraits<WarpXStreamline>::AnalysisType;
  using ArrayType = typename FlowTraits<WarpXStreamline>::ArrayType;
  using FieldType = typename FlowTraits<WarpXStreamline>::FieldType;

  VTKM_CONT WarpXStreamline() { this->SetSolverEuler(); }

  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const;

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const;

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const;

  VTKM_CONT void SetEField(const std::string& name) { this->SetActiveField(0, name); }

  VTKM_CONT void SetBField(const std::string& name) { this->SetActiveField(1, name); }

  VTKM_CONT std::string GetEField() const { return this->GetActiveFieldName(0); }

  VTKM_CONT std::string GetBField() const { return this->GetActiveFieldName(1); }
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_WarpXStreamline_h
