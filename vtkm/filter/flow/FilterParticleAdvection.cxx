//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#include <vtkm/Particle.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/FilterParticleAdvection.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT vtkm::cont::DataSet FilterParticleAdvection::DoExecute(const vtkm::cont::DataSet& inData)
{
  auto out = this->DoExecutePartitions(inData);
  if (out.GetNumberOfPartitions() != 1)
    throw vtkm::cont::ErrorFilterExecution("Wrong number of results");

  return out.GetPartition(0);
}

VTKM_CONT void FilterParticleAdvection::ValidateOptions() const
{
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");

  vtkm::Id numSeeds = this->Seeds.GetNumberOfValues();
#ifdef VTKM_ENABLE_MPI
  vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id totalNumSeeds = 0;
  vtkmdiy::mpi::all_reduce(comm, numSeeds, totalNumSeeds, std::plus<vtkm::Id>{});
  numSeeds = totalNumSeeds;
#endif
  if (numSeeds == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (!this->Seeds.IsBaseComponentType<vtkm::Particle>() &&
      !this->Seeds.IsBaseComponentType<vtkm::ChargedParticle>())
    throw vtkm::cont::ErrorFilterExecution("Unsupported particle type in seed array.");
  if (this->NumberOfSteps == 0)
    throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
  if (this->NumberOfSteps < 0)
    throw vtkm::cont::ErrorFilterExecution("NumberOfSteps cannot be negative");
  if (this->StepSize < 0)
    throw vtkm::cont::ErrorFilterExecution("StepSize cannot be negative");
}

}
}
} // namespace vtkm::filter::flow
