//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Particle.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/LagrangianStructures.h>

#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>
#include <vtkm/filter/flow/worklet/LagrangianStructures.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>

namespace
{

VTKM_CONT void MapField(vtkm::cont::DataSet& dataset, const vtkm::cont::Field& field)
{
  if (field.IsWholeDataSetField())
  {
    dataset.AddField(field);
  }
  else
  {
    // Do not currently support other types of fields.
  }
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace flow
{

namespace detail
{
class ExtractParticlePosition : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, FieldOut position);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VTKM_EXEC void operator()(const vtkm::Particle& particle, vtkm::Vec3f& pt) const
  {
    pt = particle.GetPosition();
  }
};

class MakeParticles : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn seed, FieldOut particle);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  using InputDomain = _1;

  VTKM_EXEC void operator()(const vtkm::Id index,
                            const vtkm::Vec3f& seed,
                            vtkm::Particle& particle) const
  {
    particle.SetID(index);
    particle.SetPosition(seed);
  }
};

} //detail


VTKM_CONT vtkm::cont::DataSet LagrangianStructures::DoExecute(const vtkm::cont::DataSet& input)
{
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<FieldHandle>;
  using GridEvaluator = vtkm::worklet::flow::GridEvaluator<FieldType>;
  using IntegratorType = vtkm::worklet::flow::RK4Integrator<GridEvaluator>;
  using Stepper = vtkm::worklet::flow::Stepper<IntegratorType, GridEvaluator>;

  vtkm::FloatDefault stepSize = this->GetStepSize();
  vtkm::Id numberOfSteps = this->GetNumberOfSteps();

  vtkm::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  vtkm::cont::UnknownCellSet cellset = input.GetCellSet();

  vtkm::cont::DataSet lcsInput;
  if (this->GetUseAuxiliaryGrid())
  {
    vtkm::Id3 lcsGridDims = this->GetAuxiliaryGridDimensions();
    vtkm::Bounds inputBounds = coordinates.GetBounds();
    vtkm::Vec3f origin(static_cast<vtkm::FloatDefault>(inputBounds.X.Min),
                       static_cast<vtkm::FloatDefault>(inputBounds.Y.Min),
                       static_cast<vtkm::FloatDefault>(inputBounds.Z.Min));
    vtkm::Vec3f spacing;
    spacing[0] = static_cast<vtkm::FloatDefault>(inputBounds.X.Length()) /
      static_cast<vtkm::FloatDefault>(lcsGridDims[0] - 1);
    spacing[1] = static_cast<vtkm::FloatDefault>(inputBounds.Y.Length()) /
      static_cast<vtkm::FloatDefault>(lcsGridDims[1] - 1);
    spacing[2] = static_cast<vtkm::FloatDefault>(inputBounds.Z.Length()) /
      static_cast<vtkm::FloatDefault>(lcsGridDims[2] - 1);
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
  vtkm::cont::ArrayHandle<vtkm::Vec3f> lcsInputPoints, lcsOutputPoints;
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
    const auto field = input.GetField(this->GetActiveFieldName());

    FieldType velocities(field.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>(),
                         field.GetAssociation());
    GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), velocities);
    Stepper integrator(evaluator, stepSize);
    vtkm::worklet::flow::ParticleAdvection particles;
    vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle> advectionResult;
    vtkm::cont::ArrayHandle<vtkm::Particle> advectionPoints;
    this->Invoke(detail::MakeParticles{}, lcsInputPoints, advectionPoints);
    advectionResult = particles.Run(integrator, advectionPoints, numberOfSteps);
    this->Invoke(detail::ExtractParticlePosition{}, advectionResult.Particles, lcsOutputPoints);
  }
  // FTLE output field
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outputField;
  vtkm::FloatDefault advectionTime = this->GetAdvectionTime();

  vtkm::cont::UnknownCellSet lcsCellSet = lcsInput.GetCellSet();
  if (lcsCellSet.IsType<Structured2DType>())
  {
    using AnalysisType = vtkm::worklet::flow::LagrangianStructures<2>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }
  else if (lcsCellSet.IsType<Structured3DType>())
  {
    using AnalysisType = vtkm::worklet::flow::LagrangianStructures<3>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    vtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }


  auto fieldmapper = [&](vtkm::cont::DataSet& dataset, const vtkm::cont::Field& field) {
    MapField(dataset, field);
  };
  vtkm::cont::DataSet output = this->CreateResultCoordinateSystem(
    input, lcsInput.GetCellSet(), lcsInput.GetCoordinateSystem(), fieldmapper);
  output.AddPointField(this->GetOutputFieldName(), outputField);
  return output;
}

}
}
} // namespace vtkm::filter::flow
