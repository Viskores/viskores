//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_DSIUnsteadyState_h
#define vtk_m_filter_particleadvection_DSIUnsteadyState_h

#include <vtkm/filter/particleadvection/DSI.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class DSIUnsteadyState : public vtkm::filter::particleadvection::DSI
{
public:
  DSIUnsteadyState(const vtkm::cont::DataSet& ds1,
                   const vtkm::cont::DataSet& ds2,
                   vtkm::FloatDefault t1,
                   vtkm::FloatDefault t2,
                   vtkm::Id id,
                   const vtkm::filter::particleadvection::DSI::FieldNameType& fieldName,
                   vtkm::filter::particleadvection::IntegrationSolverType solverType,
                   vtkm::filter::particleadvection::VectorFieldType vecFieldType,
                   vtkm::filter::particleadvection::ParticleAdvectionResultType resultType)
    : vtkm::filter::particleadvection::DSI(id, fieldName, solverType, vecFieldType, resultType)
    , DataSet1(ds1)
    , DataSet2(ds2)
    , Time1(t1)
    , Time2(t2)
  {
  }

  VTKM_CONT virtual inline void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                         vtkm::FloatDefault stepSize,
                                         vtkm::Id maxSteps) override;

  VTKM_CONT virtual inline void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
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

#ifndef vtk_m_filter_particleadvection_DSIUnsteadyState_hxx
#include <vtkm/filter/particleadvection/DSIUnsteadyState.hxx>
#endif

#endif //vtk_m_filter_particleadvection_DSIUnsteadyState_h
