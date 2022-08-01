//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h
#define vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>
#include <vtkm/filter/flow/worklet/TemporalGridEvaluators.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class DataSetIntegratorUnsteadyState : public vtkm::filter::flow::internal::DataSetIntegrator
{
public:
  DataSetIntegratorUnsteadyState(
    const vtkm::cont::DataSet& ds1,
    const vtkm::cont::DataSet& ds2,
    vtkm::FloatDefault t1,
    vtkm::FloatDefault t2,
    vtkm::Id id,
    const vtkm::filter::flow::internal::DataSetIntegrator::FieldNameType& fieldName,
    vtkm::filter::flow::IntegrationSolverType solverType,
    vtkm::filter::flow::VectorFieldType vecFieldType,
    vtkm::filter::flow::FlowResultType resultType)
    : vtkm::filter::flow::internal::DataSetIntegrator(id,
                                                      fieldName,
                                                      solverType,
                                                      vecFieldType,
                                                      resultType)
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
    vtkm::worklet::flow::VelocityField<ArrayType>& velocityField1,
    vtkm::worklet::flow::VelocityField<ArrayType>& velocityField2) const
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

      velocityField1 = vtkm::worklet::flow::VelocityField<ArrayType>(arr1, assoc);
      velocityField2 = vtkm::worklet::flow::VelocityField<ArrayType>(arr2, assoc);
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



namespace internal
{
using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using VelocityFieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
using UnsteadyStateGridEvalType = vtkm::worklet::flow::TemporalGridEvaluator<VelocityFieldType>;

template <typename GridEvalType, typename ParticleType>
class AdvectHelper;

template <typename ParticleType>
class AdvectHelper<UnsteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::flow::ParticleAdvectionResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::ParticleAdvection,
               vtkm::worklet::flow::ParticleAdvectionResult,
               vtkm::worklet::flow::RK4Integrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::ParticleAdvection,
               vtkm::worklet::flow::ParticleAdvectionResult,
               vtkm::worklet::flow::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::flow::StreamlineResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::Streamline,
               vtkm::worklet::flow::StreamlineResult,
               vtkm::worklet::flow::RK4Integrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::Streamline,
               vtkm::worklet::flow::StreamlineResult,
               vtkm::worklet::flow::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField1,
                       const vtkm::cont::DataSet& ds1,
                       vtkm::FloatDefault t1,
                       const VelocityFieldType& velField2,
                       const vtkm::cont::DataSet& ds2,
                       vtkm::FloatDefault t2,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType = vtkm::worklet::flow::Stepper<SolverType<UnsteadyStateGridEvalType>,
                                                     UnsteadyStateGridEvalType>;

    WorkletType worklet;
    UnsteadyStateGridEvalType eval(ds1, t1, velField1, ds2, t2, velField2);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};

}

VTKM_CONT inline void DataSetIntegratorUnsteadyState::DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                                               vtkm::FloatDefault stepSize,
                                                               vtkm::Id maxSteps)
{
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  using AHType = internal::AdvectHelper<internal::UnsteadyStateGridEvalType, vtkm::Particle>;

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
    FieldType velField1, velField2;
    this->GetVelocityFields(velField1, velField2);

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::flow::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

VTKM_CONT inline void DataSetIntegratorUnsteadyState::DoAdvect(
  DSIHelperInfo<vtkm::ChargedParticle>& vtkmNotUsed(b),
  vtkm::FloatDefault vtkmNotUsed(stepSize),
  vtkm::Id vtkmNotUsed(maxSteps))
{
}


}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegratorUnsteadyState_h
