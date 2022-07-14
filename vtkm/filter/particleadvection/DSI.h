//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DSI_h
#define vtk_m_filter_DSI_h

#include <vtkm/cont/DataSet.h>
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
class DSIStuff
{
public:
  DSIStuff(const std::vector<ParticleType>& v,
           const vtkm::filter::particleadvection::BoundsMap& boundsMap,
           const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , V(v)
  {
  }

  std::vector<ParticleType> V, A, I;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
  const vtkm::filter::particleadvection::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
};

using DSIStuffP = DSIStuff<vtkm::Particle>;
using DSIStuffCP = DSIStuff<vtkm::ChargedParticle>;
using DSIStuffType =
  vtkm::cont::internal::Variant<DSIStuff<vtkm::Particle>, DSIStuff<vtkm::ChargedParticle>>;

/*
template <typename ParticleType>
struct DSIStuff
{
  DSIStuff(const vtkm::filter::particleadvection::BoundsMap& boundsMap,
           const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
  {
  }

  DSIStuff(const std::vector<ParticleType>& v,
           const vtkm::filter::particleadvection::BoundsMap& boundsMap,
           const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , V(v)
  {
  }

  std::vector<ParticleType> V;
  std::vector<ParticleType> A, I;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
  const vtkm::filter::particleadvection::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
};
*/


class DSI
{
protected:
  using VelocityFieldNameType = std::string;
  using ElectroMagneticFieldNameType = std::pair<std::string, std::string>;
  using FieldNameType =
    vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>;

  using SType =
    vtkm::cont::internal::Variant<DSIStuff<vtkm::Particle>, DSIStuff<vtkm::ChargedParticle>>;
  using PType =
    vtkm::cont::internal::Variant<std::vector<vtkm::Particle>, std::vector<vtkm::ChargedParticle>>;
  using RType =
    vtkm::cont::internal::Variant<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>,
                                  vtkm::worklet::ParticleAdvectionResult<vtkm::ChargedParticle>,
                                  vtkm::worklet::StreamlineResult<vtkm::Particle>,
                                  vtkm::worklet::StreamlineResult<vtkm::ChargedParticle>>;



public:
  DSI(vtkm::Id id,
      const FieldNameType& fieldName,
      vtkm::filter::particleadvection::IntegrationSolverType solverType,
      vtkm::filter::particleadvection::VectorFieldType vecFieldType,
      vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : FieldName(fieldName)
    , Id(id)
    , SolverType(solverType)
    , VecFieldType(vecFieldType)
    , ResType(resultType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  /*
  VTKM_CONT bool IsSteadyState() const
  {
    return this->Data.GetIndex() == this->Data.GetIndexOf<SteadyStateDataType>();
  }
  VTKM_CONT bool IsUnsteadyState() const
  {
    return this->Data.GetIndex() == this->Data.GetIndexOf<UnsteadyStateDataType>();
  }
  */

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }


  /*
  template <typename ParticleType>
  VTKM_CONT void Advect(std::vector<ParticleType>& v,
                        vtkm::FloatDefault stepSize, //move these to member data(?)
                        vtkm::Id maxSteps,
                        DSIStuff<ParticleType>& stuff);

  template <template <typename> class ResultType, typename ParticleType>
  VTKM_CONT void
  BumHole(ResultType<ParticleType>& result)
  {
    std::cout<<"bumhole"<<std::endl;
  }
  */

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) = 0;

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::ChargedParticle>& b,
                                  vtkm::FloatDefault stepSize, //move these to member data(?)
                                  vtkm::Id maxSteps) = 0;

  VTKM_CONT
  virtual void ADVECT(DSIStuffType& b,
                      vtkm::FloatDefault stepSize, //move these to member data(?)
                      vtkm::Id maxSteps)
  {
    if (b.GetIndex() == b.GetIndexOf<DSIStuff<vtkm::Particle>>())
    {
      auto& bb = b.Get<DSIStuff<vtkm::Particle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }
    else if (b.GetIndex() == b.GetIndexOf<DSIStuff<vtkm::ChargedParticle>>())
    {
      auto& bb = b.Get<DSIStuff<vtkm::ChargedParticle>>();
      this->DoAdvect(bb, stepSize, maxSteps);
    }

    //b.CastAndCall([&] (auto& concrete) { this->DoAdvect(concrete, stepSize, maxSteps); });

    /*
    if (b.GetIndex() == b.GetIndexOf<BumP>())
    {
      std::cout<<"It's a particle!"<<std::endl;
      auto bb = b.Get<BumP>();
    }
    else if (b.GetIndex() == b.GetIndexOf<BumC>())
    {
      std::cout<<"It's a CHARGED particle!"<<std::endl;
      auto bb = b.Get<BumC>();
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Steady state velocity field vector type not available");
    */
  }

  /*
  VTKM_CONT
  virtual void DoADVECT(std::vector<PType>& v,
                        vtkm::FloatDefault stepSize,
                        vtkm::Id maxSteps,
                        SType& stuff)
  {
  }
  */

  template <typename ParticleType>
  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const;


protected:
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void UpdateResult(const ResultType<ParticleType>& result,
                              DSIStuff<ParticleType>& stuff);

  /*
  template <typename ArrayType>
  VTKM_CONT void GetSteadyStateVelocityField(
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField) const
  {
    if (this->Data.GetIndex() == this->Data.GetIndexOf<SteadyStateDataType>())
    {
      const auto& ds = this->Data.Get<SteadyStateDataType>();
      this->GetVelocityField(ds, velocityField);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Steady state velocity field vector type not available");
  }

  template <typename ArrayType>
  VTKM_CONT void GetUnsteadyStateVelocityField(
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField1,
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField2) const
  {
    if (this->Data.GetIndex() == this->Data.GetIndexOf<UnsteadyStateDataType>())
    {
      const auto& data = this->Data.Get<UnsteadyStateDataType>();
      this->GetVelocityField(data.DataSet1, velocityField1);
      this->GetVelocityField(data.DataSet2, velocityField2);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsteady state velocity field vector type not available");
  }
  */

  template <typename ArrayType>
  VTKM_CONT void GetVelocityField(
    const vtkm::cont::DataSet& ds,
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField) const
  {
    if (this->FieldName.GetIndex() == this->FieldName.GetIndexOf<VelocityFieldNameType>())
    {
      const auto& fieldNm = this->FieldName.Get<VelocityFieldNameType>();
      auto assoc = ds.GetField(fieldNm).GetAssociation();
      ArrayType arr;
      vtkm::cont::ArrayCopyShallowIfPossible(ds.GetField(fieldNm).GetData(), arr);

      velocityField = vtkm::worklet::particleadvection::VelocityField<ArrayType>(arr, assoc);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Velocity field vector type not available");
  }

  template <typename ArrayType>
  VTKM_CONT void GetElectroMagneticField(
    const vtkm::cont::DataSet& ds,
    vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>& elecMagField) const
  {
    if (this->FieldName.GetIndex() == this->FieldName.GetIndexOf<ElectroMagneticFieldNameType>())
    {
      const auto& fieldNms = this->FieldName.Get<ElectroMagneticFieldNameType>();
      auto assoc1 = ds.GetField(fieldNms.first).GetAssociation();
      auto assoc2 = ds.GetField(fieldNms.second).GetAssociation();
      if (assoc1 != assoc2)
        throw vtkm::cont::ErrorFilterExecution(
          "Electro-magnetic vector fields have differing associations");

      ArrayType arr1, arr2;
      vtkm::cont::ArrayCopyShallowIfPossible(ds.GetField(fieldNms.first).GetData(), arr1);
      vtkm::cont::ArrayCopyShallowIfPossible(ds.GetField(fieldNms.second).GetData(), arr2);

      elecMagField =
        vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>(arr1, arr2, assoc1);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Velocity field vector type not available");
  }

  /*
  template <typename ArrayType>
  VTKM_CONT void GetSteadyStateElectroMagneticField(
    vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>& elecMagField) const
  {
    if (this->Data.GetIndex() == this->Data.GetIndexOf<SteadyStateDataType>())
    {
      const auto& ds = this->Data.Get<SteadyStateDataType>();
      this->GetElectroMagneticField(ds, elecMagField);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Electro-magnetic vector field type not available");
  }

  template <typename ArrayType>
  VTKM_CONT void GetUnsteadyStateElectroMagneticField(
    vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>& emField1,
    vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>& emField2) const
  {
    if (this->Data.GetIndex() == this->Data.GetIndexOf<UnsteadyStateDataType>())
    {
      this->GetSteadyStateElectroMagneticField(emField1);
      this->GetSteadyStateElectroMagneticField(emField2);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsteady state electro-magnetic field vector type not available");
  }
  */

  //template <typename ParticleType>
  void Meow(const char* func, const int& lineNum) const;

  template <typename ParticleType>
  VTKM_CONT void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                   DSIStuff<ParticleType>& stuff) const;

  /*
  using SteadyStateDataType = vtkm::cont::DataSet;
  struct UnsteadyStateDataType
  {
    UnsteadyStateDataType(const vtkm::cont::DataSet& ds1,
                          const vtkm::cont::DataSet& ds2,
                          vtkm::FloatDefault t1,
                          vtkm::FloatDefault t2)
      : DataSet1(ds1)
      , DataSet2(ds2)
      , Time1(t1)
      , Time2(t2)
    {
    }

    vtkm::cont::DataSet DataSet1;
    vtkm::cont::DataSet DataSet2;
    vtkm::FloatDefault Time1;
    vtkm::FloatDefault Time2;
  };
*/

  //Data members.
  //  vtkm::cont::internal::Variant<SteadyStateDataType, UnsteadyStateDataType> Data;
  vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType> FieldName;

  vtkm::Id Id;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;
  vtkm::filter::particleadvection::ParticleAdvectionResultType ResType =
    vtkm::filter::particleadvection::ParticleAdvectionResultType::UNKNOWN_TYPE;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
  std::vector<RType> Results;
};


class SteadyStateDSI : public DSI
{
public:
  SteadyStateDSI(const vtkm::cont::DataSet& ds,
                 vtkm::Id id,
                 const FieldNameType& fieldName,
                 vtkm::filter::particleadvection::IntegrationSolverType solverType,
                 vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                 vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : DSI(id, fieldName, solverType, vecFieldType, resultType)
    , DataSet(ds)
  {
  }

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) override;

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::ChargedParticle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) override;

protected:
  template <typename ArrayType>
  VTKM_CONT void GetVelocityField(
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField) const
  {
    if (this->FieldName.GetIndex() == this->FieldName.GetIndexOf<VelocityFieldNameType>())
    {
      const auto& fieldNm = this->FieldName.Get<VelocityFieldNameType>();
      auto assoc = this->DataSet.GetField(fieldNm).GetAssociation();
      ArrayType arr;
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(fieldNm).GetData(), arr);

      velocityField = vtkm::worklet::particleadvection::VelocityField<ArrayType>(arr, assoc);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Velocity field vector type not available");
  }


private:
  vtkm::cont::DataSet DataSet;
};

class UnsteadyStateDSI : public DSI
{
public:
  UnsteadyStateDSI(const vtkm::cont::DataSet& ds1,
                   const vtkm::cont::DataSet& ds2,
                   vtkm::FloatDefault t1,
                   vtkm::FloatDefault t2,
                   vtkm::Id id,
                   const FieldNameType& fieldName,
                   vtkm::filter::particleadvection::IntegrationSolverType solverType,
                   vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                   vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : DSI(id, fieldName, solverType, vecFieldType, resultType)
    , DataSet1(ds1)
    , DataSet2(ds2)
    , Time1(t1)
    , Time2(t2)
  {
  }

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) override;

  VTKM_CONT virtual void DoAdvect(DSIStuff<vtkm::ChargedParticle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) override;

protected:
  template <typename ArrayType>
  VTKM_CONT void GetVelocityFields(
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField1,
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField2) const
  {
    if (this->FieldName.GetIndex() == this->FieldName.GetIndexOf<VelocityFieldNameType>())
    {
      const auto& fieldNm = this->FieldName.Get<VelocityFieldNameType>();
      auto assoc = this->DataSet1.GetField(fieldNm).GetAssociation();
      if (assoc != this->DataSet2.GetField(fieldNm).GetAssociation())
        throw vtkm::cont::ErrorFilterExecution(
          "Unsteady state velocity fields have differnt associations");

      ArrayType arr1, arr2;
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet1.GetField(fieldNm).GetData(), arr1);
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet2.GetField(fieldNm).GetData(), arr2);

      velocityField1 = vtkm::worklet::particleadvection::VelocityField<ArrayType>(arr1, assoc);
      velocityField2 = vtkm::worklet::particleadvection::VelocityField<ArrayType>(arr2, assoc);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Velocity field vector type not available");
  }

private:
  vtkm::cont::DataSet DataSet1;
  vtkm::cont::DataSet DataSet2;
  vtkm::FloatDefault Time1;
  vtkm::FloatDefault Time2;
};

}
}
}

#ifndef vtk_m_filter_particleadvection_DSI_hxx
#include <vtkm/filter/particleadvection/DSI.hxx>
#endif

#endif //vtk_m_filter_DSI_h
