//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtkm_worklet_particleadvection_termination
#define vtkm_worklet_particleadvection_termination

#include <vtkm/Types.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

class NormalTerminationExec
{
public:
  VTKM_EXEC_CONT
  NormalTerminationExec()
    : MaxSteps(0)
  {
  }

  VTKM_EXEC_CONT
  NormalTerminationExec(vtkm::Id maxSteps)
    : MaxSteps(maxSteps)
  {
  }

  template <typename ParticleType>
  VTKM_EXEC bool CheckTermination(ParticleType& particle) const
  {
    /// Checks particle properties to make a decision for termination
    /// -- Check if the particle is out of spatial boundaries
    /// -- Check if the particle has reached the maximum number of steps
    /// -- Check if the particle is in a zero velocity region
    auto& status = particle.GetStatus();
    if (particle.GetNumberOfSteps() == this->MaxSteps)
    {
      status.SetTerminate();
      particle.SetStatus(status);
    }
    bool terminate = status.CheckOk() && !status.CheckTerminate() && !status.CheckSpatialBounds() &&
      !status.CheckTemporalBounds() && !status.CheckInGhostCell() && !status.CheckZeroVelocity();
    return terminate;
  }

private:
  vtkm::Id MaxSteps;
};

class NormalTermination : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT
  NormalTermination()
    : MaxSteps(0)
  {
  }

  VTKM_CONT
  NormalTermination(vtkm::Id maxSteps)
    : MaxSteps(maxSteps)
  {
  }

  VTKM_CONT
  vtkm::Id AllocationSize() const { return this->MaxSteps; }

  VTKM_CONT vtkm::worklet::flow::NormalTerminationExec PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return vtkm::worklet::flow::NormalTerminationExec(MaxSteps);
  }

private:
  vtkm::Id MaxSteps;
};

class PoincareTerminationExec
{
public:
  VTKM_EXEC_CONT
  PoincareTerminationExec()
    : MaxSteps(0)
    , MaxPunctures(0)
  {
  }

  VTKM_EXEC_CONT
  PoincareTerminationExec(vtkm::Id maxSteps, vtkm::Id maxPunctures)
    : MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
  {
  }

  template <typename ParticleType>
  VTKM_EXEC bool CheckTermination(ParticleType& particle) const
  {
    /// Checks particle properties to make a decision for termination
    /// -- Check if the particle is out of spatial boundaries
    /// -- Check if the particle has reached the maximum number of steps
    /// -- Check if the particle is in a zero velocity region
    auto& status = particle.GetStatus();
    if (particle.GetNumberOfSteps() >= this->MaxSteps ||
        particle.GetNumberOfPunctures() >= this->MaxPunctures)
    {
      status.SetTerminate();
      particle.SetStatus(status);
    }
    bool terminate = status.CheckOk() && !status.CheckTerminate() && !status.CheckSpatialBounds() &&
      !status.CheckTemporalBounds() && !status.CheckInGhostCell() && !status.CheckZeroVelocity();
    return terminate;
  }

private:
  vtkm::Id MaxSteps;
  vtkm::Id MaxPunctures;
};

class PoincareTermination : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT
  PoincareTermination()
    : MaxSteps(0)
    , MaxPunctures(0)
  {
  }

  VTKM_CONT
  PoincareTermination(vtkm::Id maxSteps, vtkm::Id maxPunctures)
    : MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
  {
  }

  VTKM_CONT
  vtkm::Id AllocationSize() const { return this->MaxPunctures; }

  VTKM_CONT vtkm::worklet::flow::PoincareTerminationExec PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return vtkm::worklet::flow::PoincareTerminationExec(MaxSteps, MaxPunctures);
  }

private:
  vtkm::Id MaxSteps;
  vtkm::Id MaxPunctures;
};

} // namespace particleadvection
} // namespace worklet
} // namespace vtkm

#endif
