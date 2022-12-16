//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/FilterParticleAdvectionSteadyState.h>
#include <vtkm/filter/flow/internal/DataSetIntegratorSteadyState.h>

#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

namespace
{
VTKM_CONT std::vector<vtkm::filter::flow::internal::DataSetIntegratorSteadyState>
CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::cont::Variant<std::string, std::pair<std::string, std::string>>& activeField,
  const vtkm::filter::flow::internal::BoundsMap& boundsMap,
  const vtkm::filter::flow::IntegrationSolverType solverType,
  const vtkm::filter::flow::VectorFieldType vecFieldType,
  const vtkm::filter::flow::FlowResultType resultType)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorSteadyState;

  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (activeField.IsType<DSIType::VelocityFieldNameType>())
    {
      const auto& fieldNm = activeField.Get<DSIType::VelocityFieldNameType>();
      if (!ds.HasPointField(fieldNm) && !ds.HasCellField(fieldNm))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    }
    else if (activeField.IsType<DSIType::ElectroMagneticFieldNameType>())
    {
      const auto& fieldNm = activeField.Get<DSIType::ElectroMagneticFieldNameType>();
      const auto& electric = fieldNm.first;
      const auto& magnetic = fieldNm.second;
      if (!ds.HasPointField(electric) && !ds.HasCellField(electric))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
      if (!ds.HasPointField(magnetic) && !ds.HasCellField(magnetic))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    }
    dsi.emplace_back(ds, blockId, activeField, solverType, vecFieldType, resultType);
  }
  return dsi;
}

} //anonymous namespace

VTKM_CONT vtkm::cont::PartitionedDataSet FilterParticleAdvectionSteadyState::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorSteadyState;
  this->ValidateOptions();

  using VariantType = vtkm::cont::Variant<std::string, std::pair<std::string, std::string>>;
  VariantType variant;

  if (this->VecFieldType == vtkm::filter::flow::VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    const auto& field = this->GetActiveFieldName();
    variant.Emplace<DSIType::VelocityFieldNameType>(field);
  }
  else if (this->VecFieldType == vtkm::filter::flow::VectorFieldType::ELECTRO_MAGNETIC_FIELD_TYPE)
  {
    const auto& electric = this->GetEField();
    const auto& magnetic = this->GetBField();
    variant.Emplace<DSIType::ElectroMagneticFieldNameType>(electric, magnetic);
  }

  vtkm::filter::flow::internal::BoundsMap boundsMap(input);
  auto dsi = CreateDataSetIntegrators(
    input, variant, boundsMap, this->SolverType, this->VecFieldType, this->GetResultType());

  vtkm::filter::flow::internal::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->GetResultType());

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
