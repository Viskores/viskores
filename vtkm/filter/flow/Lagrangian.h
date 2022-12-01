//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_Lagrangian_h
#define vtk_m_filter_flow_Lagrangian_h

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

class VTKM_FILTER_FLOW_EXPORT Lagrangian : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  bool CanThread() const override { return false; }

  VTKM_CONT
  void SetInitFlag(bool val) { this->InitFlag = val; }

  VTKM_CONT
  void SetExtractFlows(bool val) { this->ExtractFlows = val; }

  VTKM_CONT
  void SetResetParticles(bool val) { this->ResetParticles = val; }

  VTKM_CONT
  void SetStepSize(vtkm::Float32 val) { this->StepSize = val; }

  VTKM_CONT
  void SetWriteFrequency(vtkm::Id val) { this->WriteFrequency = val; }

  VTKM_CONT
  void SetSeedResolutionInX(vtkm::Id val) { this->ResX = val; }

  VTKM_CONT
  void SetSeedResolutionInY(vtkm::Id val) { this->ResY = val; }

  VTKM_CONT
  void SetSeedResolutionInZ(vtkm::Id val) { this->ResZ = val; }

  VTKM_CONT
  void SetCustomSeedResolution(vtkm::Id val) { this->CustRes = val; }

  VTKM_CONT
  void SetSeedingResolution(vtkm::Id3 val) { this->SeedRes = val; }

  VTKM_CONT
  void UpdateSeedResolution(vtkm::cont::DataSet input);

  VTKM_CONT
  void InitializeSeedPositions(const vtkm::cont::DataSet& input);

  VTKM_CONT
  void SetCycle(vtkm::Id cycle) { this->Cycle = cycle; }
  VTKM_CONT
  vtkm::Id GetCycle() const { return this->Cycle; }

  VTKM_CONT
  void SetBasisParticles(const vtkm::cont::ArrayHandle<vtkm::Particle>& basisParticles)
  {
    this->BasisParticles = basisParticles;
  }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Particle> GetBasisParticles() const { return this->BasisParticles; }

  VTKM_CONT
  void SetBasisParticlesOriginal(const vtkm::cont::ArrayHandle<vtkm::Particle>& basisParticles)
  {
    this->BasisParticlesOriginal = basisParticles;
  }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Particle> GetBasisParticlesOriginal() const
  {
    return this->BasisParticlesOriginal;
  }

  VTKM_CONT
  void SetBasisParticleValidity(const vtkm::cont::ArrayHandle<vtkm::Id>& valid)
  {
    this->BasisParticlesValidity = valid;
  }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Id> GetBasisParticleValidity() const
  {
    return this->BasisParticlesValidity;
  }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

  vtkm::cont::ArrayHandle<vtkm::Particle> BasisParticles;
  vtkm::cont::ArrayHandle<vtkm::Particle> BasisParticlesOriginal;
  vtkm::cont::ArrayHandle<vtkm::Id> BasisParticlesValidity;
  vtkm::Id CustRes = 0;
  vtkm::Id Cycle = 0;
  bool ExtractFlows = false;
  bool InitFlag = true;
  bool ResetParticles = true;
  vtkm::Id ResX = 1;
  vtkm::Id ResY = 1;
  vtkm::Id ResZ = 1;
  vtkm::FloatDefault StepSize;
  vtkm::Id3 SeedRes = { 1, 1, 1 };
  vtkm::Id WriteFrequency = 0;
};

}
}
} //vtkm::filter::flow

#endif // #define vtk_m_filter_flow_Lagrangian_h
