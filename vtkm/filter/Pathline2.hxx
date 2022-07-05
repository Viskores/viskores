//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Pathline2_hxx
#define vtk_m_filter_Pathline2_hxx

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/Pathline2.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionAlgorithm.h>

#include <vtkm/filter/particleadvection/DSI.h>
#include <vtkm/filter/particleadvection/PAV.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Pathline2::Pathline2()
  : vtkm::filter::FilterTemporalParticleAdvection<Pathline2, vtkm::Particle>()
{
  this->ResultType = vtkm::filter::particleadvection::ParticleAdvectionResultType::STREAMLINE_TYPE;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Pathline2::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using DSIType = vtkm::filter::particleadvection::DSI;

  this->ValidateOptions();
  //Make sure everything matches up ok.
  this->VecFieldType = vtkm::filter::particleadvection::VectorFieldType::VELOCITY_FIELD_TYPE;

  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  std::string activeField = this->GetActiveFieldName();

  std::vector<DSIType*> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds1 = input.GetPartition(i);
    auto ds2 = this->DataSet2.GetPartition(i);
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

  this->SeedArray = this->Seeds;
  vtkm::filter::particleadvection::PAV<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->ResultType);
  return pav.Execute(this->NumberOfSteps, this->StepSize, this->SeedArray);
}

VTKM_CONT vtkm::cont::DataSet Pathline3::DoExecute(const vtkm::cont::DataSet& inData)
{
  std::cout << "Meow DS" << std::endl;
  auto result = this->DoExecutePartitions(inData);
  return result.GetPartition(0);
}

VTKM_CONT vtkm::cont::PartitionedDataSet Pathline3::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& inData)
{
  std::cout << "Meow pDS" << std::endl;
  return inData;
}

}
} // namespace vtkm::filter
#endif
