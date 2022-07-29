//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h
#define vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h

#include <vtkm/filter/flow/internal/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class DataSetIntegratorSteadyState : public vtkm::filter::flow::internal::DataSetIntegrator
{
public:
  DataSetIntegratorSteadyState(const vtkm::cont::DataSet& ds,
                               vtkm::Id id,
                               const FieldNameType& fieldName,
                               vtkm::filter::flow::IntegrationSolverType solverType,
                               vtkm::filter::flow::VectorFieldType vecFieldType,
                               vtkm::filter::flow::FlowResultType resultType)
    : vtkm::filter::flow::internal::DataSetIntegrator(id,
                                                      fieldName,
                                                      solverType,
                                                      vecFieldType,
                                                      resultType)
    , DataSet(ds)
  {
  }

  VTKM_CONT inline void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                 vtkm::FloatDefault stepSize,
                                 vtkm::Id maxSteps) override;

  VTKM_CONT inline void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
                                 vtkm::FloatDefault stepSize,
                                 vtkm::Id maxSteps) override;

protected:
  template <typename ArrayType>
  VTKM_CONT void GetVelocityField(
    vtkm::worklet::flow::VelocityField<ArrayType>& velocityField) const
  {
    if (this->FieldName.GetIndex() == this->FieldName.GetIndexOf<VelocityFieldNameType>())
    {
      const auto& fieldNm = this->FieldName.Get<VelocityFieldNameType>();
      auto assoc = this->DataSet.GetField(fieldNm).GetAssociation();
      ArrayType arr;
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(fieldNm).GetData(), arr);

      velocityField = vtkm::worklet::flow::VelocityField<ArrayType>(arr, assoc);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Velocity field vector type not available");
  }

private:
  vtkm::cont::DataSet DataSet;
};


namespace internal
{
using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using VelocityFieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
using SteadyStateGridEvalType = vtkm::worklet::flow::GridEvaluator<VelocityFieldType>;

template <typename GridEvalType, typename ParticleType>
class AdvectHelper;

template <typename ParticleType>
class AdvectHelper<SteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
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
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::ParticleAdvection,
               vtkm::worklet::flow::ParticleAdvectionResult,
               vtkm::worklet::flow::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
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
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::Streamline,
               vtkm::worklet::flow::StreamlineResult,
               vtkm::worklet::flow::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField,
                       const vtkm::cont::DataSet& ds,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::flow::Stepper<SolverType<SteadyStateGridEvalType>, SteadyStateGridEvalType>;

    WorkletType worklet;
    SteadyStateGridEvalType eval(ds, velField);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};
}

VTKM_CONT inline void DataSetIntegratorSteadyState::DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                                             vtkm::FloatDefault stepSize,
                                                             vtkm::Id maxSteps)
{
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
    FieldType velField;
    this->GetVelocityField(velField);

    using AHType = internal::AdvectHelper<internal::SteadyStateGridEvalType, vtkm::Particle>;

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::flow::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

VTKM_CONT inline void DataSetIntegratorSteadyState::DoAdvect(
  DSIHelperInfo<vtkm::ChargedParticle>& vtkmNotUsed(b),
  vtkm::FloatDefault vtkmNotUsed(stepSize),
  vtkm::Id vtkmNotUsed(maxSteps))
{
}

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h
