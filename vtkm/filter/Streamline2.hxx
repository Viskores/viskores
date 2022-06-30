//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Streamline2_hxx
#define vtk_m_filter_Streamline2_hxx

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/Streamline2.h>
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
inline VTKM_CONT Streamline2::Streamline2()
  : vtkm::filter::FilterParticleAdvection<Streamline2, vtkm::Particle>()
{
  this->ResultType = vtkm::filter::particleadvection::STREAMLINE_TYPE;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Streamline2::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //  return input;

#if 1
  using DSIType = vtkm::filter::particleadvection::DSI;

  this->ValidateOptions();
  //Make sure everything matches up ok.
  this->VecFieldType = vtkm::filter::particleadvection::VELOCITY_FIELD_TYPE;

  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
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

  this->SeedArray = this->Seeds;
  vtkm::filter::particleadvection::PAV<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->ResultType);
  return pav.Execute(this->NumberOfSteps, this->StepSize, this->SeedArray);
#endif
}

VTKM_CONT vtkm::cont::DataSet Streamline3::DoExecute(const vtkm::cont::DataSet& inData)
{
  std::cout << "Meow DS" << std::endl;
  auto result = this->DoExecutePartitions(inData);
  return result.GetPartition(0);
}

VTKM_CONT vtkm::cont::PartitionedDataSet Streamline3::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& inData)
{
  std::cout << "Meow pDS" << std::endl;
  return inData;
}

}
} // namespace vtkm::filter
#endif
