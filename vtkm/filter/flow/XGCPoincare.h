//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_XGCPoincare_h
#define vtk_m_filter_flow_XGCPoincare_h

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

class XGCPoincare;

template <>
struct FlowTraits<XGCPoincare>
{
  using ParticleType = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::PoincareTermination;
  using AnalysisType = vtkm::worklet::flow::XGCPoincare<ParticleType>;
  using CompArrayType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using VecArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::XGCField<CompArrayType, VecArrayType>;
};

/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class VTKM_FILTER_FLOW_EXPORT XGCPoincare
  : public vtkm::filter::flow::FilterParticleAdvectionSteadyState<XGCPoincare>
{
public:
  using ParticleType = typename FlowTraits<XGCPoincare>::ParticleType;
  using TerminationType = typename FlowTraits<XGCPoincare>::TerminationType;
  using AnalysisType = typename FlowTraits<XGCPoincare>::AnalysisType;
  using CompArrayType = typename FlowTraits<XGCPoincare>::CompArrayType;
  using VecArrayType = typename FlowTraits<XGCPoincare>::VecArrayType;
  using FieldType = typename FlowTraits<XGCPoincare>::FieldType;

  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const;

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const;

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const;

  VTKM_CONT void SetXGCParams(const vtkm::worklet::flow::XGCParams& params)
  {
    this->Params = params;
  }
  VTKM_CONT vtkm::worklet::flow::XGCParams GetXGCParams() const { return this->Params; }

  VTKM_CONT void SetPeriod(vtkm::FloatDefault period) { this->Period = period; }
  VTKM_CONT vtkm::FloatDefault GetPeriod() const { return this->Period; }

  VTKM_CONT void SetMaxPunctures(vtkm::Id maxPunctures) { this->MaxPunctures = maxPunctures; }
  VTKM_CONT vtkm::Id GetMaxPunctures() const { return this->MaxPunctures; }

  // As_ff
  VTKM_CONT void SetField1(const std::string& name) { this->SetActiveField(0, name); }
  // dAs_ff
  VTKM_CONT void SetField2(const std::string& name) { this->SetActiveField(1, name); }
  // Bfield
  VTKM_CONT void SetField3(const std::string& name) { this->SetActiveField(2, name); }
  // Psi
  VTKM_CONT void SetField4(const std::string& name) { this->SetActiveField(3, name); }
  // coeff_1d
  VTKM_CONT void SetField5(const std::string& name) { this->SetActiveField(4, name); }
  // coeff_2d
  VTKM_CONT void SetField6(const std::string& name) { this->SetActiveField(5, name); }

  VTKM_CONT std::string GetField1() const { return this->GetActiveFieldName(0); }
  VTKM_CONT std::string GetField2() const { return this->GetActiveFieldName(1); }
  VTKM_CONT std::string GetField3() const { return this->GetActiveFieldName(2); }
  VTKM_CONT std::string GetField4() const { return this->GetActiveFieldName(3); }
  VTKM_CONT std::string GetField5() const { return this->GetActiveFieldName(4); }
  VTKM_CONT std::string GetField6() const { return this->GetActiveFieldName(5); }

private:
  vtkm::worklet::flow::XGCParams Params;
  vtkm::FloatDefault Period;
  vtkm::Id MaxPunctures;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_XGCPoincare_h
