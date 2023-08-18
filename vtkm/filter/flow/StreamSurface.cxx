//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/StreamSurface.h>
#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>
#include <vtkm/filter/flow/worklet/StreamSurface.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT vtkm::cont::DataSet StreamSurface::DoExecute(const vtkm::cont::DataSet& input)
{
  //Validate inputs.
  if (this->GetUseCoordinateSystemAsField())
    throw vtkm::cont::ErrorFilterExecution("Coordinate system as field not supported");
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  if (!this->Seeds.IsBaseComponentType<vtkm::Particle>() &&
      this->Seeds.IsBaseComponentType<vtkm::ChargedParticle>())
    throw vtkm::cont::ErrorFilterExecution("Unsupported particle type in seed array.");
  if (this->NumberOfSteps == 0)
    throw vtkm::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw vtkm::cont::ErrorFilterExecution("Step size not specified.");
  if (this->NumberOfSteps < 0)
    throw vtkm::cont::ErrorFilterExecution("NumberOfSteps cannot be negative");
  if (this->StepSize < 0)
    throw vtkm::cont::ErrorFilterExecution("StepSize cannot be negative");

  if (!this->Seeds.IsBaseComponentType<vtkm::Particle>())
    throw vtkm::cont::ErrorFilterExecution("Unsupported seed type in StreamSurface filter.");

  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::flow::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::flow::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::flow::Stepper<RK4Type, GridEvalType>;

  //compute streamlines
  const auto& field = input.GetField(this->GetActiveFieldName());
  FieldHandle arr;
  vtkm::cont::ArrayCopyShallowIfPossible(field.GetData(), arr);
  FieldType velocities(arr, field.GetAssociation());
  GridEvalType eval(coords, cells, velocities);
  Stepper rk4(eval, this->StepSize);

  using ParticleArray = vtkm::cont::ArrayHandle<vtkm::Particle>;
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds.AsArrayHandle<ParticleArray>(), seedArray);

  vtkm::worklet::flow::ParticleAdvection worklet;
  vtkm::worklet::flow::NormalTermination termination(this->NumberOfSteps);
  vtkm::worklet::flow::StreamlineAnalysis<vtkm::Particle> analysis(this->NumberOfSteps);

  worklet.Run(rk4, seedArray, termination, analysis);

  //compute surface from streamlines
  vtkm::worklet::flow::StreamSurface streamSurface;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> srfPoints;
  vtkm::cont::CellSetSingleType<> srfCells;
  vtkm::cont::CoordinateSystem slCoords("coordinates", analysis.Streams);
  streamSurface.Run(slCoords, analysis.PolyLines, srfPoints, srfCells);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outputCoords("coordinates", srfPoints);
  outData.AddCoordinateSystem(outputCoords);
  outData.SetCellSet(srfCells);

  return outData;
}

}
}
} // namespace vtkm::filter::flow
