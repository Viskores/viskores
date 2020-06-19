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
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

/// \brief Copy fields in vtkm::Particle to standard types.
///
/// Given an \c ArrayHandle of vtkm::Particle, this function copies the
/// position field into an \c ArrayHandle of \c Vec3f objects.
///
VTKM_CONT_EXPORT
VTKM_CONT void ParticleArrayCopy(
  const vtkm::cont::ArrayHandle<vtkm::Particle, vtkm::cont::StorageTagBasic>& inP,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos);

/// \brief Copy all fields in vtkm::Particle to standard types.
///
/// Given an \c ArrayHandle of vtkm::Particle, this function copies the
/// position, ID, number of steps, status and time into a separate
/// \c ArrayHandle.
///
VTKM_CONT_EXPORT
VTKM_CONT void ParticleArrayCopy(
  const vtkm::cont::ArrayHandle<vtkm::Particle, vtkm::cont::StorageTagBasic>& inP,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outID,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outSteps,
  vtkm::cont::ArrayHandle<vtkm::ParticleStatus, vtkm::cont::StorageTagBasic>& outStatus,
  vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>& outTime);
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ParticleArrayCopy_h
