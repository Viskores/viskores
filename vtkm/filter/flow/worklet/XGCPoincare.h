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

#ifndef vtkm_worklet_particleadvection_xgcpoincare_h
#define vtkm_worklet_particleadvection_xgcpoincare_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/filter/flow/worklet/XGCHelper.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
class XGCPoincareExec
{
public:
  VTKM_EXEC_CONT
  XGCPoincareExec()
    : NumParticles(0)
    , MaxSteps(0)
    , MaxPunctures(0)
    , PunctureCounts()
    , Validity()
    , PunctureIDs()
    , OutputR()
    , OutputZ()
    , OutputTheta()
    , OutputPsi()
  {
  }

  VTKM_CONT
  XGCPoincareExec(vtkm::Id numParticles,
                  vtkm::Id maxSteps,
                  vtkm::Id maxPunctures,
                  const XGCParams& params,
                  vtkm::FloatDefault period,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coeff_1D,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coeff_2D,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& punctureCounts,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& validity,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& punctureIDs,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& outputR,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& outputZ,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& outputTheta,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& outputPsi,
                  vtkm::cont::DeviceAdapterId device,
                  vtkm::cont::Token& token)
    : NumParticles(numParticles)
    , MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
    , Params(params)
    , Period(period)
  {
    Coeff_1D = coeff_1D.PrepareForInput(device, token);
    Coeff_2D = coeff_2D.PrepareForInput(device, token);
    PunctureCounts = punctureCounts.PrepareForInPlace(device, token);
    Validity = validity.PrepareForInPlace(device, token);
    PunctureIDs = punctureIDs.PrepareForInPlace(device, token);
    OutputR = outputR.PrepareForOutput(this->NumParticles * this->MaxPunctures, device, token);
    OutputZ = outputZ.PrepareForOutput(this->NumParticles * this->MaxPunctures, device, token);
    OutputTheta =
      outputTheta.PrepareForOutput(this->NumParticles * this->MaxPunctures, device, token);
    OutputPsi = outputPsi.PrepareForOutput(this->NumParticles * this->MaxPunctures, device, token);
  }

  //template <typename ParticleType>
  VTKM_EXEC void Analyze(const vtkm::Id index,
                         const ParticleType& oldParticle,
                         ParticleType& newParticle) const
  {
    vtkm::Id numRevs0 = vtkm::Floor(vtkm::Abs(oldParticle.Pos[1] / this->Period));
    vtkm::Id numRevs1 = vtkm::Floor(vtkm::Abs(newParticle.Pos[1] / this->Period));

    //if (this->SaveTraces)
    //  Intersections.Set(idx*this->MaxIter + particle.NumSteps, particle.Pos);
    //std::cout<<std::setprecision(12)<<" Step: "<<particle.Pos<<std::endl;
    if (numRevs1 > numRevs0)
    {
      auto R = newParticle.Pos[0];
      auto Z = newParticle.Pos[2];
      auto theta = vtkm::ATan2(Z - this->Params.eq_axis_z, R - this->Params.eq_axis_r);
      if (theta < 0)
        theta += vtkm::TwoPi();

      //calcualte psi. need to stash psi on the particle somehow....
      vtkm::Vec3f ptRPZ = newParticle.Pos;
      ParticleMetaData metadata;
      HighOrderB(ptRPZ, metadata, this->Params, this->Coeff_1D, this->Coeff_2D);

      vtkm::Id loc = (index * this->MaxPunctures) + newParticle.NumPunctures;
      this->Validity.Set(loc, 1);
      this->OutputR.Set(loc, R);
      this->OutputZ.Set(loc, Z);
      this->OutputTheta.Set(loc, theta);
      this->OutputPsi.Set(loc, metadata.Psi / this->Params.eq_x_psi);
      this->PunctureIDs.Set(loc, index);
      newParticle.NumPunctures++;
    }
  }

private:
  using ScalarPortalR = typename vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType;
  using ScalarPortalW = typename vtkm::cont::ArrayHandle<vtkm::FloatDefault>::WritePortalType;

  using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;
  using VecPortal = typename vtkm::cont::ArrayHandle<vtkm::Vec3f>::WritePortalType;

  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;
  vtkm::Id MaxPunctures;
  vtkm::FloatDefault Period;
  XGCParams Params;
  ScalarPortalR Coeff_1D;
  ScalarPortalR Coeff_2D;

  ScalarPortalW OutputR;
  ScalarPortalW OutputZ;
  ScalarPortalW OutputTheta;
  ScalarPortalW OutputPsi;
  IdPortal PunctureIDs;

  //VecPortal Intersections;
  IdPortal PunctureCounts;
  IdPortal Validity;
};

template <typename ParticleType>
class XGCPoincare : public vtkm::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  vtkm::cont::ArrayHandle<ParticleType> Particles;
  vtkm::cont::ArrayHandle<vtkm::Id> PunctureIDs;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> OutputR;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> OutputZ;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> OutputTheta;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> OutputPsi;

  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

  VTKM_CONT
  XGCPoincare(vtkm::Id maxSteps,
              vtkm::Id maxPunctures,
              const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coeff_1D,
              const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& coeff_2D,
              const XGCParams& params,
              vtkm::FloatDefault period)
    : Particles()
    , MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
    , Coeff_1D(coeff_1D)
    , Coeff_2D(coeff_2D)
    , Params(params)
    , Period(period)
  {
  }

  VTKM_CONT
  void UseAsTemplate(const XGCPoincare& other)
  {
    this->MaxSteps = other.MaxSteps;
    this->MaxPunctures = other.MaxPunctures;
    this->Period = other.Period;
    this->Params = other.Params;
    this->Coeff_1D = other.Coeff_1D;
    this->Coeff_2D = other.Coeff_2D;
  }

  VTKM_CONT XGCPoincareExec<ParticleType> PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                              vtkm::cont::Token& token) const
  {
    return XGCPoincareExec<ParticleType>(this->NumParticles,
                                         this->MaxSteps,
                                         this->MaxPunctures,
                                         this->Params,
                                         this->Period,
                                         this->Coeff_1D,
                                         this->Coeff_2D,
                                         this->PunctureCounts,
                                         this->Validity,
                                         this->PunctureIDs,
                                         this->OutputR,
                                         this->OutputZ,
                                         this->OutputTheta,
                                         this->OutputPsi,
                                         device,
                                         token);
  }

  VTKM_CONT bool SupportPushOutOfBounds() const { return false; }

  VTKM_CONT
  void InitializeAnalysis(const vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    this->NumParticles = particles.GetNumberOfValues();
    //Create Validity initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> validity(0, this->NumParticles * this->MaxPunctures);
    vtkm::cont::ArrayCopy(validity, this->Validity);
    //Create PunctureCount initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> punctureCounts(0, this->NumParticles);
    vtkm::cont::ArrayCopy(punctureCounts, this->PunctureCounts);
    vtkm::cont::ArrayHandleConstant<vtkm::Id> initIds(-1, this->NumParticles * this->MaxPunctures);
    vtkm::cont::ArrayCopy(initIds, this->PunctureIDs);
  }

  VTKM_CONT
  void FinalizeAnalysis(vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    (void)particles;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> outR, outZ, outTheta, outPsi;
    vtkm::cont::ArrayHandle<vtkm::Id> outIds;

    vtkm::cont::Algorithm::CopyIf(this->OutputR, this->Validity, outR, IsOne());
    vtkm::cont::Algorithm::CopyIf(this->OutputZ, this->Validity, outZ, IsOne());
    vtkm::cont::Algorithm::CopyIf(this->OutputTheta, this->Validity, outTheta, IsOne());
    vtkm::cont::Algorithm::CopyIf(this->OutputPsi, this->Validity, outPsi, IsOne());
    vtkm::cont::Algorithm::CopyIf(this->PunctureIDs, this->Validity, outIds, IsOne());

    vtkm::cont::ArrayCopyShallowIfPossible(outR, this->OutputR);
    vtkm::cont::ArrayCopyShallowIfPossible(outZ, this->OutputZ);
    vtkm::cont::ArrayCopyShallowIfPossible(outTheta, this->OutputTheta);
    vtkm::cont::ArrayCopyShallowIfPossible(outPsi, this->OutputPsi);
    vtkm::cont::ArrayCopyShallowIfPossible(outIds, this->PunctureIDs);
  }

  VTKM_CONT static bool MakeDataSet(vtkm::cont::DataSet& dataset,
                                    const std::vector<XGCPoincare>& results)
  {
    (void)dataset;
    (void)results;
    return true;
  }

private:
  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;
  vtkm::Id MaxPunctures;
  // For Higher order interpolation
  // on RZ uniform grid, not the triangle grid.
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Coeff_1D;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Coeff_2D;

  XGCParams Params;
  vtkm::FloatDefault Period;

  // Output Arrays
  vtkm::cont::ArrayHandle<vtkm::Id> PunctureCounts;
  //vtkm::cont::ArrayHandle<vtkm::Id> InitialPunctureCounts;
  vtkm::cont::ArrayHandle<vtkm::Id> Validity;
};


} // namespace particleadvection
} // namespace worklet
} // namespace vtkm

#endif //vtkm_worklet_particleadvection_xgcpoincare_h
