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

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class DataSetIntegratorSteadyState
  : public vtkm::filter::flow::internal::DataSetIntegrator<DataSetIntegratorSteadyState>
{
public:
  DataSetIntegratorSteadyState(const vtkm::cont::DataSet& ds,
                               vtkm::Id id,
                               const FieldNameType& fieldName,
                               vtkm::filter::flow::IntegrationSolverType solverType,
                               vtkm::filter::flow::VectorFieldType vecFieldType,
                               vtkm::filter::flow::FlowResultType resultType)
    : vtkm::filter::flow::internal::DataSetIntegrator<DataSetIntegratorSteadyState>(id,
                                                                                    fieldName,
                                                                                    solverType,
                                                                                    vecFieldType,
                                                                                    resultType)
    , DataSet(ds)
  {
  }

  VTKM_CONT inline void DoAdvect(DSIHelperInfo<vtkm::Particle>& b,
                                 vtkm::FloatDefault stepSize,
                                 vtkm::Id maxSteps);

  VTKM_CONT inline void DoAdvect(DSIHelperInfo<vtkm::ChargedParticle>& b,
                                 vtkm::FloatDefault stepSize,
                                 vtkm::Id maxSteps);

protected:
  template <typename ArrayType>
  VTKM_CONT void GetVelocityField(
    vtkm::worklet::flow::VelocityField<ArrayType>& velocityField) const
  {
    if (this->FieldName.IsType<VelocityFieldNameType>())
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

  template <typename ArrayType>
  VTKM_CONT void GetElectroMagneticField(
    vtkm::worklet::flow::ElectroMagneticField<ArrayType>& ebField) const
  {
    if (this->FieldName.IsType<ElectroMagneticFieldNameType>())
    {
      const auto& fieldNm = this->FieldName.Get<ElectroMagneticFieldNameType>();
      const auto& electric = fieldNm.first;
      const auto& magnetic = fieldNm.second;
      auto eAssoc = this->DataSet.GetField(electric).GetAssociation();
      auto bAssoc = this->DataSet.GetField(magnetic).GetAssociation();

      if (eAssoc != bAssoc)
      {
        throw vtkm::cont::ErrorFilterExecution("E and B field need to have same association");
      }
      ArrayType eField, bField;
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(electric).GetData(), eField);
      vtkm::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(magnetic).GetData(), bField);
      ebField = vtkm::worklet::flow::ElectroMagneticField<ArrayType>(eField, bField, eAssoc);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Electromagnetic field vector type not available");
  }

private:
  vtkm::cont::DataSet DataSet;
};


namespace internal
{
using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

using VelocityFieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
using VelocityEvalType = vtkm::worklet::flow::GridEvaluator<VelocityFieldType>;

using EBFieldType = vtkm::worklet::flow::ElectroMagneticField<ArrayType>;
using EBEvalType = vtkm::worklet::flow::GridEvaluator<EBFieldType>;

//template <typename GridEvalType, typename ParticleType>
//class AdvectHelper;

template <typename FieldType, typename ParticleType>
class AdvectHelper //<FieldType, ParticleType>
{
public:
  using SteadyStateGridEvalType = vtkm::worklet::flow::GridEvaluator<FieldType>;

  static void Advect(const FieldType& vecField,
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
        vecField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::ParticleAdvection,
               vtkm::worklet::flow::ParticleAdvectionResult,
               vtkm::worklet::flow::EulerIntegrator>(
        vecField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const FieldType& vecField,
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
        vecField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::flow::Streamline,
               vtkm::worklet::flow::StreamlineResult,
               vtkm::worklet::flow::EulerIntegrator>(
        vecField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const FieldType& vecField,
                       const vtkm::cont::DataSet& ds,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::flow::Stepper<SolverType<SteadyStateGridEvalType>, SteadyStateGridEvalType>;

    WorkletType worklet;
    SteadyStateGridEvalType eval(ds, vecField);
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
    FieldType vecField;
    this->GetVelocityField(vecField);

    using AHType = internal::AdvectHelper<internal::VelocityFieldType, vtkm::Particle>;

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(
        vecField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::flow::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(
        vecField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

VTKM_CONT inline void DataSetIntegratorSteadyState::DoAdvect(
  DSIHelperInfo<vtkm::ChargedParticle>& b,
  vtkm::FloatDefault stepSize,
  vtkm::Id maxSteps)
{
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  if (this->VecFieldType == VectorFieldType::ELECTRO_MAGNETIC_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::flow::ElectroMagneticField<ArrayType>;
    FieldType ebField;
    this->GetElectroMagneticField(ebField);

    using AHType = internal::AdvectHelper<internal::EBFieldType, vtkm::ChargedParticle>;

    if (this->IsParticleAdvectionResult())
    {
      vtkm::worklet::flow::ParticleAdvectionResult<vtkm::ChargedParticle> result;
      AHType::Advect(
        ebField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else if (this->IsStreamlineResult())
    {
      vtkm::worklet::flow::StreamlineResult<vtkm::ChargedParticle> result;
      AHType::Advect(
        ebField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported result type");
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported vector field type");
}

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegratorSteadyState_h
