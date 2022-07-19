//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DataSetIntegrator_h
#define vtk_m_filter_particleadvection_DataSetIntegrator_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/EulerIntegrator.h>
#include <vtkm/worklet/particleadvection/IntegratorStatus.h>
#include <vtkm/worklet/particleadvection/RK4Integrator.h>
#include <vtkm/worklet/particleadvection/Stepper.h>

#include <vtkm/cont/internal/Variant.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(const std::vector<ParticleType>& v,
                const vtkm::filter::particleadvection::BoundsMap& boundsMap,
                const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , V(v)
  {
  }

  const vtkm::filter::particleadvection::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;

  std::vector<ParticleType> A, I, V;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
};

using DSIHelperInfoType = vtkm::cont::internal::Variant<DSIHelperInfo<vtkm::Particle>,
                                                        DSIHelperInfo<vtkm::ChargedParticle>>;

class DataSetIntegrator
{
protected:
  using VelocityFieldNameType = std::string;
  using ElectroMagneticFieldNameType = std::pair<std::string, std::string>;
  using FieldNameType =
    vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>;

  using RType =
    vtkm::cont::internal::Variant<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>,
                                  vtkm::worklet::ParticleAdvectionResult<vtkm::ChargedParticle>,
                                  vtkm::worklet::StreamlineResult<vtkm::Particle>,
                                  vtkm::worklet::StreamlineResult<vtkm::ChargedParticle>>;

public:
  DataSetIntegrator(vtkm::Id id,
                    const FieldNameType& fieldName,
                    vtkm::filter::particleadvection::IntegrationSolverType solverType,
                    vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                    vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : FieldName(fieldName)
    , Id(id)
    , SolverType(solverType)
    , VecFieldType(vecFieldType)
    , AdvectionResType(resultType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VTKM_CONT
  virtual void Advect(DSIHelperInfoType& b,
                      vtkm::FloatDefault stepSize, //move these to member data(?)
                      vtkm::Id maxSteps)
  {
    if (b.GetIndex() == b.GetIndexOf<DSIHelperInfo<vtkm::Particle>>())
    {
      auto& bb = b.Get<DSIHelperInfo<vtkm::Particle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }
    else if (b.GetIndex() == b.GetIndexOf<DSIHelperInfo<vtkm::ChargedParticle>>())
    {
      auto& bb = b.Get<DSIHelperInfo<vtkm::ChargedParticle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }

    //b.CastAndCall([&] (auto& concrete) { this->DoAdvect(concrete, stepSize, maxSteps); });
  }

  template <typename ParticleType>
  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const;


protected:
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void UpdateResult(const ResultType<ParticleType>& result,
                              DSIHelperInfo<ParticleType>& dsiInfo);

protected:
  VTKM_CONT bool IsParticleAdvectionResult() const
  {
    return this->AdvectionResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE;
  }

  VTKM_CONT bool IsStreamlineResult() const
  {
    return this->AdvectionResType == ParticleAdvectionResultType::STREAMLINE_TYPE;
  }

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) = 0;

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) = 0;

  template <typename ParticleType>
  VTKM_CONT void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                   DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType> FieldName;

  vtkm::Id Id;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;
  vtkm::filter::particleadvection::ParticleAdvectionResultType AdvectionResType =
    vtkm::filter::particleadvection::ParticleAdvectionResultType::UNKNOWN_TYPE;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
  std::vector<RType> Results;
};


}
}
}

#ifndef vtk_m_filter_particleadvection_DataSetIntegrator_hxx
#include <vtkm/filter/particleadvection/DataSetIntegrator.hxx>
#endif

#endif //vtk_m_filter_particleadvection_DataSetIntegrator_h
