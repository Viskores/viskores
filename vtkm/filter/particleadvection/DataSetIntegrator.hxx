//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetIntegrator_hxx
#define vtk_m_filter_DataSetIntegrator_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<
  vtkm::worklet::particleadvection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>;
using TemporalGridEvalType = vtkm::worklet::particleadvection::TemporalGridEvaluator<
  vtkm::worklet::particleadvection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>;
using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
using TemporalRK4Type = vtkm::worklet::particleadvection::RK4Integrator<TemporalGridEvalType>;

//-----
// Specialization for ParticleAdvection worklet
template <>
template <>
inline void DataSetIntegratorBase<GridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const RK4Type& rk4,
  vtkm::Id maxSteps,
  vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>& result) const
{
  vtkm::worklet::ParticleAdvection Worklet;
  result = Worklet.Run(rk4, seeds, maxSteps);
}

//-----
// Specialization for Streamline worklet
template <>
template <>
inline void DataSetIntegratorBase<GridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const RK4Type& rk4,
  vtkm::Id maxSteps,
  vtkm::worklet::StreamlineResult<vtkm::Particle>& result) const
{
  vtkm::worklet::Streamline Worklet;
  result = Worklet.Run(rk4, seeds, maxSteps);
}

//-----
// Specialization for Pathline worklet
template <>
template <>
inline void DataSetIntegratorBase<TemporalGridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const TemporalRK4Type& rk4,
  vtkm::Id maxSteps,
  vtkm::worklet::StreamlineResult<vtkm::Particle>& result) const
{
  vtkm::worklet::Streamline Worklet;
  result = Worklet.Run(rk4, seeds, maxSteps);
}

}
}
} // namespace vtkm::filter::particleadvection

#endif
