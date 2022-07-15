//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DSISteadyState_h
#define vtk_m_filter_particleadvection_DSISteadyState_h

#include <vtkm/filter/particleadvection/DSI.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class DSISteadyState : public vtkm::filter::particleadvection::DSI
{
public:
  DSISteadyState(const vtkm::cont::DataSet& ds,
                 vtkm::Id id,
                 const FieldNameType& fieldName,
                 vtkm::filter::particleadvection::IntegrationSolverType solverType,
                 vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                 vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : vtkm::filter::particleadvection::DSI(id, fieldName, solverType, vecFieldType, resultType)
    , DataSet(ds)
  {
  }

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                  vtkm::FloatDefault stepSize,
                                  vtkm::Id maxSteps) override;

  VTKM_CONT virtual void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
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

}
}
}

#include <vtkm/filter/particleadvection/DSISteadyState.hxx>

#endif //vtk_m_filter_particleadvection_DSISteadyState_h
