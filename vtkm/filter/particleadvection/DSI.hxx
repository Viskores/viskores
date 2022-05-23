//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DSI_hxx
#define vtk_m_filter_DSI_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DSI::Advect(std::vector<ParticleType>& v,
                           vtkm::FloatDefault stepSize,
                           vtkm::Id maxSteps,
                           ResultType<ParticleType>& result) const
{
  //using Association = vtkm::cont::Field::Association;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(v, copyFlag);

  std::cout << "DSI::Advect() " << v.size() << std::endl;

  if (this->SolverType == IntegrationSolverType::RK4_TYPE)
  {
    if (this->VecFieldType == VELOCITY_FIELD_TYPE) //vtkm::Particle, VelocityField
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

      FieldType velField;
      this->GetVelocityField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);
      vtkm::worklet::ParticleAdvection Worklet;
      result = Worklet.Run(stepper, seedArray, maxSteps);
      //Put results in unknown array??
    }
    else if (this->VecFieldType == ELECTRO_MAGNETIC_FIELD_TYPE) //vtkm::ChargedParticle
    {
      using FieldType = vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

      FieldType velField;
      this->GetElectroMagneticField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);
      vtkm::worklet::ParticleAdvection Worklet;
      result = Worklet.Run(stepper, seedArray, maxSteps);
    }
  }

  else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
  {
    if (this->VecFieldType == VELOCITY_FIELD_TYPE) //vtkm::Particle, VelocityField
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

      FieldType velField;
      this->GetVelocityField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);
      vtkm::worklet::ParticleAdvection Worklet;
      result = Worklet.Run(stepper, seedArray, maxSteps);
      //Put results in unknown array??
    }
  }
}

}
}
}

#endif //vtk_m_filter_DSI_hxx
