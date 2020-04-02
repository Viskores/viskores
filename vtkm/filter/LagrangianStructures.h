//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_LagrangianStructures_h
#define vtk_m_filter_LagrangianStructures_h

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/filter/FilterDataSetWithField.h>

#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>

namespace vtkm
{
namespace filter
{

class LagrangianStructures : public vtkm::filter::FilterDataSetWithField<LagrangianStructures>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  LagrangianStructures();

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

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::FloatDefault StepSize;
  vtkm::Id NumberOfSteps;
  vtkm::FloatDefault AdvectionTime;
  bool UseAuxiliaryGrid = false;
  vtkm::Id3 AuxiliaryDims;
  bool UseFlowMapOutput = false;
  std::string OutputFieldName;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> FlowMapOutput;
};

} // namespace filter
} // namespace vtkm

#include <vtkm/filter/LagrangianStructures.hxx>

#endif // vtk_m_filter_LagrangianStructures_h
