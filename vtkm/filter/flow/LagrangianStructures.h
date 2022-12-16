//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_LagrangianStructures_h
#define vtk_m_filter_flow_LagrangianStructures_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

class VTKM_FILTER_FLOW_EXPORT LagrangianStructures : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  bool CanThread() const override { return false; }

  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }
  vtkm::FloatDefault GetStepSize() { return this->StepSize; }

  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }
  vtkm::Id GetNumberOfSteps() { return this->NumberOfSteps; }

  void SetAdvectionTime(vtkm::FloatDefault advectionTime) { this->AdvectionTime = advectionTime; }
  vtkm::FloatDefault GetAdvectionTime() { return this->AdvectionTime; }

  void SetUseAuxiliaryGrid(bool useAuxiliaryGrid) { this->UseAuxiliaryGrid = useAuxiliaryGrid; }
  bool GetUseAuxiliaryGrid() { return this->UseAuxiliaryGrid; }

  void SetAuxiliaryGridDimensions(vtkm::Id3 auxiliaryDims) { this->AuxiliaryDims = auxiliaryDims; }
  vtkm::Id3 GetAuxiliaryGridDimensions() { return this->AuxiliaryDims; }

  void SetUseFlowMapOutput(bool useFlowMapOutput) { this->UseFlowMapOutput = useFlowMapOutput; }
  bool GetUseFlowMapOutput() { return this->UseFlowMapOutput; }

  void SetOutputFieldName(std::string outputFieldName) { this->OutputFieldName = outputFieldName; }
  std::string GetOutputFieldName() { return this->OutputFieldName; }

  inline void SetFlowMapOutput(vtkm::cont::ArrayHandle<vtkm::Vec3f>& flowMap)
  {
    this->FlowMapOutput = flowMap;
  }
  inline vtkm::cont::ArrayHandle<vtkm::Vec3f> GetFlowMapOutput() { return this->FlowMapOutput; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

  vtkm::FloatDefault AdvectionTime;
  vtkm::Id3 AuxiliaryDims;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> FlowMapOutput;
  std::string OutputFieldName = "FTLE";
  vtkm::FloatDefault StepSize = 1.0f;
  vtkm::Id NumberOfSteps = 0;
  bool UseAuxiliaryGrid = false;
  bool UseFlowMapOutput = false;
};

}
}
} // namespace vtkm

#endif // vtk_m_filter_flow_LagrangianStructures_h
