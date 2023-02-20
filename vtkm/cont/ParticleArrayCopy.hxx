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
  vtkm::Vec3f operator()(const ParticleType& p) const { return p.GetPosition(); }
};

template <typename ParticleType>
struct ExtractTerminatedFunctor
{
  VTKM_EXEC_CONT
  bool operator()(const ParticleType& p) const { return p.GetStatus().CheckTerminate(); }
};

template <typename ParticleType>
struct ExtractIDFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(const ParticleType& p) const { return p.GetID(); }
};

template <typename ParticleType>
struct ExtractStepsFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(const ParticleType& p) const { return p.GetNumberOfSteps(); }
};

template <typename ParticleType>
struct ExtractNumIntFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(const ParticleType& p) const { return p.NumIntegrations; }
};

template <typename ParticleType>
struct ExtractNumRoundsFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(const ParticleType& p) const { return p.NumRounds; }
};

template <typename ParticleType>
struct ExtractNumCommFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(const ParticleType& p) const { return p.NumCommunications; }
};

template <typename ParticleType>
struct ExtractLifeTimeFunctor
{
  VTKM_EXEC_CONT
  vtkm::FloatDefault operator()(const ParticleType& p) const { return p.LifeTime; }
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
    outPos = inParticle.GetPosition();
    outID = inParticle.GetID();
    outSteps = inParticle.GetNumberOfSteps();
    outStatus = inParticle.GetStatus();
    outTime = inParticle.GetTime();
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
    vtkm::cont::Algorithm::Copy(posTrn, outPos);
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

template <typename ParticleType>
VTKM_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const std::vector<vtkm::cont::ArrayHandle<ParticleType, vtkm::cont::StorageTagBasic>>& inputs,
  vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagBasic>& outPos,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outIDs,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outSteps,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outNumInt,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outNumRounds,
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& outNumComm,
  vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>& outLifeTime)
{
  vtkm::Id numParticles = 0;
  for (const auto& v : inputs)
    numParticles += v.GetNumberOfValues();

  outPos.Allocate(numParticles);
  outIDs.Allocate(numParticles);
  outSteps.Allocate(numParticles);
  outNumInt.Allocate(numParticles);
  outNumRounds.Allocate(numParticles);
  outNumComm.Allocate(numParticles);
  outLifeTime.Allocate(numParticles);

  vtkm::Id idx = 0;
  for (const auto& v : inputs)
  {
    auto posTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractPositionFunctor<ParticleType>());
    auto idsTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractIDFunctor<ParticleType>());
    auto stepsTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractStepsFunctor<ParticleType>());
    auto numIntTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractNumIntFunctor<ParticleType>());
    auto numRoundsTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractNumRoundsFunctor<ParticleType>());
    auto numCommTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractNumCommFunctor<ParticleType>());
    auto numLifeTimeTrn =
      vtkm::cont::make_ArrayHandleTransform(v, detail::ExtractLifeTimeFunctor<ParticleType>());

    vtkm::Id n = posTrn.GetNumberOfValues();
    vtkm::cont::Algorithm::CopySubRange(posTrn, 0, n, outPos, idx);
    vtkm::cont::Algorithm::CopySubRange(stepsTrn, 0, n, outSteps, idx);
    vtkm::cont::Algorithm::CopySubRange(idsTrn, 0, n, outIDs, idx);
    vtkm::cont::Algorithm::CopySubRange(numIntTrn, 0, n, outNumInt, idx);
    vtkm::cont::Algorithm::CopySubRange(numCommTrn, 0, n, outNumComm, idx);
    vtkm::cont::Algorithm::CopySubRange(numRoundsTrn, 0, n, outNumRounds, idx);
    vtkm::cont::Algorithm::CopySubRange(numLifeTimeTrn, 0, n, outLifeTime, idx);
    idx += n;
  }
}


}
} // namespace vtkm::cont

#endif //vtk_m_cont_ParticleArrayCopy_hxx
