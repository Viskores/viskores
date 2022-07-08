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
struct DSIStuff
{
  DSIStuff(const vtkm::filter::particleadvection::BoundsMap& boundsMap,
           const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
  {
  }

  std::vector<ParticleType> A, I;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
  const vtkm::filter::particleadvection::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
};

class DSI
{
  using VelocityFieldNameType = std::string;
  using ElectroMagneticFieldNameType = std::pair<std::string, std::string>;
  using FieldNameType =
    vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>;


public:
  DSI(const vtkm::cont::DataSet& ds,
      vtkm::Id id,
      const FieldNameType& fieldName,
      vtkm::filter::particleadvection::IntegrationSolverType solverType,
      vtkm::filter::particleadvection::VectorFieldType vecFieldType,
      vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : Data(ds)
    , FieldName(fieldName)
    , Id(id)
    , SolverType(solverType)
    , VecFieldType(vecFieldType)
    , ResType(resultType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  DSI(const vtkm::cont::DataSet& ds1,
      const vtkm::cont::DataSet& ds2,
      vtkm::FloatDefault t1,
      vtkm::FloatDefault t2,
      vtkm::Id id,
      const vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType>&
        fieldNm,
      vtkm::filter::particleadvection::IntegrationSolverType solverType,
      vtkm::filter::particleadvection::VectorFieldType vecFieldType,
      vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : Data(UnsteadyStateDataType(ds1, ds2, t1, t2))
    , FieldName(fieldNm)
    , Id(id)
    , SolverType(solverType)
    , VecFieldType(vecFieldType)
    , ResType(resultType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VTKM_CONT bool IsSteadyState() const
  {
    return this->Data.GetIndex() == this->Data.GetIndexOf<SteadyStateDataType>();
  }
  VTKM_CONT bool IsUnsteadyState() const
  {
    return this->Data.GetIndex() == this->Data.GetIndexOf<UnsteadyStateDataType>();
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }


  template <typename ParticleType>
  VTKM_CONT void Advect(std::vector<ParticleType>& v,
                        vtkm::FloatDefault stepSize, //move these to member data(?)
                        vtkm::Id maxSteps,
                        DSIStuff<ParticleType>& stuff);

  template <typename ParticleType>
  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& ds) const;

protected:
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void UpdateResult(const ResultType<ParticleType>& result,
                              DSIStuff<ParticleType>& stuff);

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
      throw vtkm::cont::ErrorFilterExecution(
        "Steady state velocity field vector type not available");
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
      throw vtkm::cont::ErrorFilterExecution(
        "Unsteady state velocity field vector type not available");
  }

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
      throw vtkm::cont::ErrorFilterExecution(
        "Unsteady state electro-magnetic field vector type not available");
  }

  //template <typename ParticleType>
  void Meow(const char* func, const int& lineNum) const;

  template <typename ParticleType>
  VTKM_CONT void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                   DSIStuff<ParticleType>& stuff) const;


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

  //Data members.
  vtkm::cont::internal::Variant<SteadyStateDataType, UnsteadyStateDataType> Data;
  vtkm::cont::internal::Variant<VelocityFieldNameType, ElectroMagneticFieldNameType> FieldName;

  vtkm::Id Id;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;
  vtkm::filter::particleadvection::ParticleAdvectionResultType ResType =
    vtkm::filter::particleadvection::ParticleAdvectionResultType::UNKNOWN_TYPE;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;

  bool CopySeedArray = false;

  using RType =
    vtkm::cont::internal::Variant<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>,
                                  vtkm::worklet::ParticleAdvectionResult<vtkm::ChargedParticle>,
                                  vtkm::worklet::StreamlineResult<vtkm::Particle>,
                                  vtkm::worklet::StreamlineResult<vtkm::ChargedParticle>>;
  std::vector<RType> Results;
};


}
}
}

#ifndef vtk_m_filter_particleadvection_DSI_hxx
#include <vtkm/filter/particleadvection/DSI.hxx>
#endif

#endif //vtk_m_filter_DSI_h
