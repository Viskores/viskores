//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Streamline_hxx
#define vtk_m_filter_Streamline_hxx

#include <vtkm/filter/Streamline.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
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
inline VTKM_CONT Streamline::Streamline()
  : vtkm::filter::FilterDataSetWithField<Streamline>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void Streamline::SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Streamline::PrepareForExecution(
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

  vtkm::filter::particleadvection::StreamlineAlgorithm sa(boundsMap, dsi);
  sa.SetNumberOfSteps(this->NumberOfSteps);
  sa.SetStepSize(this->StepSize);
  sa.SetSeeds(this->Seeds);

  sa.Go();
  vtkm::cont::PartitionedDataSet output = sa.GetOutput();
  return output;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool Streamline::MapFieldOntoOutput(vtkm::cont::DataSet&,
                                                     const vtkm::cont::Field&,
                                                     vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
