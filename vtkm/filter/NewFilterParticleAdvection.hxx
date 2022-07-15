//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_NewFilterParticleAdvection_hxx
#define vtk_m_filter_FilterParticleAdvection_hxx

#include <vtkm/filter/NewFilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT NewFilterParticleAdvection::NewFilterParticleAdvection(
  vtkm::filter::particleadvection::ParticleAdvectionResultType rType)
  : vtkm::filter::NewFilterField()
  , NumberOfSteps(0)
  , ResultType(rType)
  , SolverType(vtkm::filter::particleadvection::IntegrationSolverType::RK4_TYPE)
  , StepSize(0)
  , UseThreadedAlgorithm(false)
  , VecFieldType(vtkm::filter::particleadvection::VectorFieldType::VELOCITY_FIELD_TYPE)
{
}

void NewFilterParticleAdvection::ValidateOptions() const
{
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (this->NumberOfSteps == 0)
    throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
}

VTKM_CONT std::vector<vtkm::filter::particleadvection::DSISteadyState*>
NewFilterSteadyStateParticleAdvection::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
{
  using DSIType = vtkm::filter::particleadvection::DSISteadyState;

  std::string activeField = this->GetActiveFieldName();

  std::vector<DSIType*> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (!ds.HasPointField(activeField) && !ds.HasCellField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

    dsi.push_back(new DSIType(
      ds, blockId, activeField, this->SolverType, this->VecFieldType, this->ResultType));
  }

  return dsi;
}

VTKM_CONT std::vector<vtkm::filter::particleadvection::DSIUnsteadyState*>
NewFilterUnsteadyStateParticleAdvection::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
{
  using DSIType = vtkm::filter::particleadvection::DSIUnsteadyState;

  std::string activeField = this->GetActiveFieldName();

  std::vector<DSIType*> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds1 = input.GetPartition(i);
    auto ds2 = this->Input2.GetPartition(i);
    if ((!ds1.HasPointField(activeField) && !ds1.HasCellField(activeField)) ||
        (!ds2.HasPointField(activeField) && !ds2.HasCellField(activeField)))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

    dsi.push_back(new DSIType(ds1,
                              ds2,
                              this->Time1,
                              this->Time2,
                              blockId,
                              activeField,
                              this->SolverType,
                              this->VecFieldType,
                              this->ResultType));
  }

  return dsi;
}

}
} // namespace vtkm::filter
#endif
