//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleAdvection_hxx
#define vtk_m_filter_ParticleAdvection_hxx

#include <vtkm/filter/ParticleAdvection.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ParticleAdvection::ParticleAdvection()
  : vtkm::filter::FilterDataSetWithField<ParticleAdvection>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void ParticleAdvection::SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}


//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet ParticleAdvection::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");

  std::string activeField = this->GetActiveFieldName();
  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;
  std::vector<DataSetIntegratorType> dsi;

  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    dsi.push_back(DataSetIntegratorType(input.GetPartition(i), blockId, activeField));
  }

  vtkm::filter::particleadvection::ParticleAdvectionAlgorithm pa(boundsMap, dsi);
  pa.SetNumberOfSteps(this->NumberOfSteps);
  pa.SetStepSize(this->StepSize);
  pa.SetSeeds(this->Seeds);

  pa.Go();
  vtkm::cont::PartitionedDataSet output = pa.GetOutput();
  return output;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool ParticleAdvection::MapFieldOntoOutput(vtkm::cont::DataSet&,
                                                            const vtkm::cont::Field&,
                                                            vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
