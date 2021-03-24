//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_ParticleArrayCopy_hxx
#define vtk_m_cont_ParticleArrayCopy_hxx

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename ParticleType>
struct ExtractPositionFunctor
{
  VTKM_EXEC_CONT
  vtkm::Vec3f operator()(const ParticleType& p) const { return p.Pos; }
};

template <typename ParticleType>
struct ExtractTerminatedFunctor
{
  VTKM_EXEC_CONT
  bool operator()(const ParticleType& p) const { return p.Status.CheckTerminate(); }
};

template <typename ParticleType>
struct CopyParticleAllWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inParticle,
                                FieldOut outPos,
                                FieldOut outID,
                                FieldOut outSteps,
                                FieldOut outStatus,
                                FieldOut outTime);

  VTKM_EXEC void operator()(const ParticleType& inParticle,
                            vtkm::Vec3f& outPos,
                            vtkm::Id& outID,
                            vtkm::Id& outSteps,
                            vtkm::ParticleStatus& outStatus,
                            vtkm::FloatDefault& outTime) const
  {
    outPos = inParticle.Pos;
    outID = inParticle.ID;
    outSteps = inParticle.NumSteps;
    outStatus = inParticle.Status;
    outTime = inParticle.Time;
  }
};

} // namespace detail


template <typename ParticleType>
VTKM_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const vtkm::cont::ArrayHandle<ParticleType, vtkm::cont::StorageTagBasic>& inP,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos,
  bool CopyTerminatedOnly)
{
  auto posTrn =
    vtkm::cont::make_ArrayHandleTransform(inP, detail::ExtractPositionFunctor<ParticleType>());

  if (CopyTerminatedOnly)
  {
    auto termTrn =
      vtkm::cont::make_ArrayHandleTransform(inP, detail::ExtractTerminatedFunctor<ParticleType>());
    vtkm::cont::Algorithm::CopyIf(posTrn, termTrn, outPos);
  }
  else
    vtkm::cont::ArrayCopy(posTrn, outPos);
}


template <typename ParticleType>
VTKM_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const std::vector<vtkm::cont::ArrayHandle<ParticleType, vtkm::cont::StorageTagBasic>>& inputs,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos)
{
  vtkm::Id numParticles = 0;
  for (const auto& v : inputs)
    numParticles += v.GetNumberOfValues();
  outPos.Allocate(numParticles);

  vtkm::Id idx = 0;
  for (const auto& v : inputs)
  {
    auto posTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractPositionFunctor<ParticleType>());
    vtkm::Id n = posTrn.GetNumberOfValues();
    vtkm::cont::Algorithm::CopySubRange(posTrn, 0, n, outPos, idx);
    idx += n;
  }
}


/// \brief Copy all fields in vtkm::Particle to standard types.
///
/// Given an \c ArrayHandle of vtkm::Particle, this function copies the
/// position, ID, number of steps, status and time into a separate
/// \c ArrayHandle.
///

template <typename ParticleType>
VTKM_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const vtkm::cont::ArrayHandle<ParticleType, vtkm::cont::StorageTagBasic>& inP,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outID,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outSteps,
  vtkm::cont::ArrayHandle<vtkm::ParticleStatus, vtkm::cont::StorageTagBasic>& outStatus,
  vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>& outTime)
{
  vtkm::cont::Invoker invoke;
  detail::CopyParticleAllWorklet<ParticleType> worklet;

  invoke(worklet, inP, outPos, outID, outSteps, outStatus, outTime);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ParticleArrayCopy_hxx
