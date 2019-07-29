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

#include <vtkm/worklet/LagrangianStructures.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT LagrangianStructures::LagrangianStructures()
  : vtkm::filter::FilterDataSetWithField<LagrangianStructures>()
{
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

  Scalar stepLength = this->GetStepLength();
  vtkm::Id numberOfSteps = this->GetNumberOfStep();

  vtkm::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  vtkm::cont::DynamicCellSet cellset = input.GetCellSet();

  vtkm::cont::Dataset lcsInput;
  if (this->GetUseAuxiliaryGrid())
  {
    vtkm::Id3 lcsGridDims = this->GetAuxiliaryGridDimensions();
    vtkm::Bounds inputBounds = this->input.GetBounds();
    vtkm::Vec<Scalar, 3> origin(inputBounds.X.Min, inputBounds.Y.Min, inputBounds.Z.Min);
    vtkm::Vec<Scalar, 3> spacing;
    spacing[0] = inputBounds.X.Length() / static_cast<Scalar>(lcsGridDims[0] - 1);
    spacing[1] = inputBounds.Y.Length() / static_cast<Scalar>(lcsGridDims[1] - 1);
    spacing[2] = inputBounds.Z.Length() / static_cast<Scalar>(lcsGridDims[2] - 1);
    vtkm::cont::DataSetBuilderUniform uniformDatasetBuilder;
    lcsInput = uniformDatasetBuilder.Create(lcsGridDims, origin, spacing);
  }
  else
  {
    // Check if input dataset is structured.
    // If not, we cannot proceed.
    if (!(cellset.IsType<Structured2DType>() || cellset.IsType<Structured3DType>()))
      throw vtkm::cont::ErrorFilterExecution("Provided data is not structured, 
                                              provide parameters for an auxiliary grid.");
    lcsInput = input;
  }
  vtkm::cont::ArrayHandle<Vector> lcsInputPoints, lcsOutputPoints;
  vtkm::cont::ArrayCopy(lcsInput.GetCoordinateSystem().GetData(), lcsInputPoints);
  if (this->GetUseFlowMapOutput())
  {
    // Check if there is a 1:1 correspondense between the flow map
    // and the input points
    lcsOutputPoints = this->GetFlowMapOutput();
    if (lcsInputPoints.GetNumberOfValues() != lcsOutputPoints.GetNumberOfValues())
      throw vtkm::cont::ErrorFilterExecution("Provided flow map does not correspond
                                              to the input points for LCS filter.");
  }
  else
  {
    std::cout << "Advecting particles" << std::endl;
    GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), field);
    Integrator integrator(evaluator, stepLength);
    vtkm::worklet::ParticleAdvection particles;
    vtkm::worklet::ParticleAdvectionResult advectionResult;
    vtkm::cont::ArrayHandle<Vector> advectionPoints;
    vtkm::cont::ArrayCopy(lcsInputPoints, advectionPoints);
    advectionResult = particles.Run(integrator, advectionPoints, numberOfSteps);
    std::cout << "Advected particles" << std::endl;
    lcsOutputPoints = advectionResult.positions;
  }
  // FTLE output field
  vtkm::cont::ArrayHandle<Scalar> outputField;
  std::cout << "Calculating FTLE field" << std::endl;
  vtkm::FloatDefault advectionTime = this->GetAdvectionTime();

  vtkm::cont::DynamicCellSet lcsCellSet = lcsInput.GetCellSet();
  if (lcsCellSet.IsType<Structured2DType>())
  {
    using AnalysisType = vtkm::worklet::LagrangianStructures<2>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }
  else if (lcsCellSet.IsType<Structured3DType>())
  {
    using AnalysisType = vtkm::worklet::LagrangianStructures<3>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }

  std::cout << "Calculated FTLE field" << std::endl;
  vtkm::cont::DataSet output;
  vtkm::cont::DataSetFieldAdd fieldAdder;
  output.AddCoordinateSystem(lcsInput.GetCoordinateSystem());
  output.AddCellSet(lcsInput.GetCellSet());
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
