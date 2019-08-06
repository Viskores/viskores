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
  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvaluator = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using Integrator = vtkm::worklet::particleadvection::RK4Integrator<GridEvaluator>;

  Scalar stepSize = this->GetStepSize();
  vtkm::Id numberOfSteps = this->GetNumberOfSteps();

  vtkm::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  vtkm::cont::DynamicCellSet cellset = input.GetCellSet();

  vtkm::cont::DataSet lcsInput;
  if (this->GetUseAuxiliaryGrid())
  {
    vtkm::Id3 lcsGridDims = this->GetAuxiliaryGridDimensions();
    vtkm::Bounds inputBounds = coordinates.GetBounds();
    Vector origin(static_cast<Scalar>(inputBounds.X.Min),
                  static_cast<Scalar>(inputBounds.Y.Min),
                  static_cast<Scalar>(inputBounds.Z.Min));
    Vector spacing;
    spacing[0] =
      static_cast<Scalar>(inputBounds.X.Length()) / static_cast<Scalar>(lcsGridDims[0] - 1);
    spacing[1] =
      static_cast<Scalar>(inputBounds.Y.Length()) / static_cast<Scalar>(lcsGridDims[1] - 1);
    spacing[2] =
      static_cast<Scalar>(inputBounds.Z.Length()) / static_cast<Scalar>(lcsGridDims[2] - 1);
    vtkm::cont::DataSetBuilderUniform uniformDatasetBuilder;
    lcsInput = uniformDatasetBuilder.Create(lcsGridDims, origin, spacing);
  }
  else
  {
    // Check if input dataset is structured.
    // If not, we cannot proceed.
    if (!(cellset.IsType<Structured2DType>() || cellset.IsType<Structured3DType>()))
      throw vtkm::cont::ErrorFilterExecution(
        "Provided data is not structured, provide parameters for an auxiliary grid.");
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
      throw vtkm::cont::ErrorFilterExecution(
        "Provided flow map does not correspond to the input points for LCS filter.");
  }
  else
  {
    GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), field);
    Integrator integrator(evaluator, stepSize);
    vtkm::worklet::ParticleAdvection particles;
    vtkm::worklet::ParticleAdvectionResult advectionResult;
    vtkm::cont::ArrayHandle<Vector> advectionPoints;
    vtkm::cont::ArrayCopy(lcsInputPoints, advectionPoints);
    advectionResult = particles.Run(integrator, advectionPoints, numberOfSteps);
    lcsOutputPoints = advectionResult.positions;
  }
  // FTLE output field
  vtkm::cont::ArrayHandle<Scalar> outputField;
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
