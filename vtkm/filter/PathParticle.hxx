//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_PathParticle_hxx
#define vtk_m_filter_PathParticle_hxx

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/PathParticle.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>

#include <vtkm/filter/particleadvection/DSIUnsteadyState.h>
#include <vtkm/filter/particleadvection/PAV.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT vtkm::cont::PartitionedDataSet PathParticle::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::particleadvection::DSIUnsteadyState;
  this->ValidateOptions();

  vtkm::filter::particleadvection::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

  vtkm::filter::particleadvection::PAV<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->ResultType);

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
} // namespace vtkm::filter
#endif
