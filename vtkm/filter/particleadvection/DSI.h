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

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class DSI
{
public:
  DSI(const vtkm::cont::DataSet& ds,
      vtkm::Id id,
      const std::string& fieldNm,
      vtkm::filter::particleadvection::IntegrationSolverType solverType,
      vtkm::filter::particleadvection::VectorFieldType vecFieldType)
  {
    this->SolverType = solverType;
    this->VecFieldType = vecFieldType;
    this->DataSet = ds;
    this->Id = id;
    this->FieldName = fieldNm;

    //check that things are valid.
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  //template <typename ParticleType, typename ResultType>
  template <typename ParticleType, template <typename> class ResultType>
  VTKM_CONT void Advect(std::vector<ParticleType>& v,
                        vtkm::FloatDefault stepSize, //move these to member data(?)
                        vtkm::Id maxSteps,
                        ResultType<ParticleType>& result) const;

  template <typename ArrayType>
  VTKM_CONT void GetVelocityField(
    vtkm::worklet::particleadvection::VelocityField<ArrayType>& velocityField) const
  {
    auto assoc = this->DataSet.GetField(this->FieldName).GetAssociation();
    ArrayType arr;
    vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(this->FieldName).GetData(), arr);

    velocityField = vtkm::worklet::particleadvection::VelocityField<ArrayType>(arr, assoc);
  }

  template <typename ArrayType>
  VTKM_CONT void GetElectroMagneticField(
    vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>& elecMagField) const
  {
    ArrayType arr1, arr2;
    vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(this->FieldName).GetData(), arr1);
    vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(this->FieldName).GetData(),
                                           arr2); //fieldname 2
    auto assoc = this->DataSet.GetField(this->FieldName).GetAssociation();

    elecMagField =
      vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>(arr1, arr2, assoc);
  }

  //private:
  vtkm::cont::DataSet DataSet;
  vtkm::Id Id;
  std::string FieldName;
  vtkm::filter::particleadvection::IntegrationSolverType SolverType;
  vtkm::filter::particleadvection::VectorFieldType VecFieldType;
  bool CopySeedArray = false;
  //Add results array?
};

}
}
}

#ifndef vtk_m_filter_particleadvection_DSI_hxx
#include <vtkm/filter/particleadvection/DSI.hxx>
#endif

#endif //vtk_m_filter_DSI_h
