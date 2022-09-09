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
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

class VTKM_FILTER_FLOW_EXPORT Lagrangian : public vtkm::filter::NewFilterField
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

protected: // make this protected so the deprecated version can override.
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

private:
  VTKM_CONT
  void InitializeCoordinates(const vtkm::cont::DataSet& input,
                             std::vector<Float64>& xC,
                             std::vector<Float64>& yC,
                             std::vector<Float64>& zC);

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


//Deprecated Lagrangian filter
namespace vtkm
{
namespace filter
{

class VTKM_FILTER_FLOW_EXPORT VTKM_DEPRECATED(
  1.9,
  "Use vtkm::filter::flow::Lagrangian. "
  "Note that the new version of the filter no longer relies on global "
  "variables to record particle position from one time step to the next. "
  "It is important to keep a reference to _the same object_. "
  "If you create a new filter object, the seeds will be reinitialized.") Lagrangian
  : public vtkm::filter::flow::Lagrangian
{
private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
};

}
} //vtkm::filter



#endif // #define vtk_m_filter_flow_Lagrangian_h
