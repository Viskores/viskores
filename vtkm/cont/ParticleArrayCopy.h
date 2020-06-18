//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ParticleArrayCopy_h
#define vtk_m_cont_ParticleArrayCopy_h

#include <vtkm/Particle.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// \brief Copy fields in vtkm::Particle to standard types.
///
/// Given an \c ArrayHandle of vtkm::Particle, this function copies the
/// position field into an \c ArrayHandle of \c Vec3f objects.
///

template <typename InStorage, typename OutStorage>
VTKM_CONT void ParticleArrayCopy(const vtkm::cont::ArrayHandle<vtkm::Particle, InStorage>& inP,
                                 vtkm::cont::ArrayHandle<vtkm::Vec3f, OutStorage>& outPos);

/// \brief Copy all fields in vtkm::Particle to standard types.
///
/// Given an \c ArrayHandle of vtkm::Particle, this function copies the
/// position, ID, number of steps, status and time into a separate
/// \c ArrayHandle.
///

template <typename InStorage, typename OutStorage>
VTKM_CONT void ParticleArrayCopy(
  const vtkm::cont::ArrayHandle<vtkm::Particle, InStorage>& inP,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, OutStorage>& outPos,
  vtkm::cont::ArrayHandle<vtkm::Id, OutStorage>& outID,
  vtkm::cont::ArrayHandle<vtkm::Id, OutStorage>& outSteps,
  vtkm::cont::ArrayHandle<vtkm::ParticleStatus, OutStorage>& outStatus,
  vtkm::cont::ArrayHandle<vtkm::FloatDefault, OutStorage>& outTime);
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ParticleArrayCopy_h
