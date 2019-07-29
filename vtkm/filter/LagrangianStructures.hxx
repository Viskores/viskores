//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT LagrangianStructures::LagrangianStructures()
  : vtkm::filter::FilterDataSetWithField<LagrangianStructures>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void LagrangianStructures::SetSeeds(
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet LagrangianStructures::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  vtkm::cont::ArrayHandle<Point> points;
  vtkm::cont::ArrayHandle<Point> vectors;
  // From the sample dataset extract points and vectors
  vtkm::cont::ArrayCopy(input.GetCoordinateSystem().GetData(), points);
  input.GetField(variableName, vtkm::cont::Field::Association::POINTS).GetData().CopyTo(vectors);

  using FieldHandle = vtkm::cont::ArrayHandle<Point>;
  using GridEvaluator = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using Integrator = vtkm::worklet::particleadvection::RK4Integrator<GridEvaluator>;

  Scalar stepLength = static_cast<Scalar>(0.025);
  vtkm::Id numberOfSteps = 500;

  vtkm::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  vtkm::cont::DynamicCellSet cellset = input.GetCellSet();

  GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), vectors);
  Integrator integrator(evaluator, stepLength);

  vtkm::worklet::ParticleAdvection particles;
  vtkm::worklet::ParticleAdvectionResult res;

  std::cout << "Advecting particles" << std::endl;
  res = particles.Run(integrator, points, numberOfSteps);
  std::cout << "Advected particles" << std::endl;

  vtkm::cont::ArrayHandle<Point> outputPoints;
  vtkm::cont::ArrayCopy(res.positions, outputPoints);
  vtkm::cont::ArrayCopy(input.GetCoordinateSystem().GetData(), points);

  // FTLE output field
  vtkm::cont::ArrayHandle<Scalar> outputField;
  //std::cout << "Calculating FTLE field" << std::endl;
  vtkm::FloatDefault advectionTime = stepLength * numberOfSteps;
  std::cout << "Calculating FTLE" << std::endl;
  detail::CalculateLCSField(cellset, points, outputPoints, advectionTime, outputField);
  std::cout << "Calculated FTLE" << std::endl;

  if (cellSet.IsType<Structured2DType>())
  {
    using AnalysisType = detail::FTLECalc<2>;
    AnalysisType ftleCalculator(advectionTime, cellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(inputPoints, outputPoints, outputField);
  }
  else if (cellSet.IsType<Structured3DType>())
  {
    using AnalysisType = detail::FTLECalc<3>;
    AnalysisType ftleCalculator(advectionTime, cellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(inputPoints, outputPoints, outputField);
  }

  vtkm::cont::DataSet output;
  vtkm::cont::DataSetFieldAdd fieldAdder;
  output.AddCoordinateSystem(input.GetCoordinateSystem());
  output.AddCellSet(input.GetCellSet());
  fieldAdder.AddPointField(output, "FTLE", outputField);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool LagrangianStructures::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<T, StorageType>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
