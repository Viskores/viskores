//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/StreamSurface.h>
#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/Particles.h>
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
  this->ValidateOptions();

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

  vtkm::worklet::flow::Streamline streamline;

  using ParticleArray = vtkm::cont::ArrayHandle<vtkm::Particle>;
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds.AsArrayHandle<ParticleArray>(), seedArray);
  auto res = streamline.Run(rk4, seedArray, this->NumberOfSteps);

  //compute surface from streamlines
  vtkm::worklet::flow::StreamSurface streamSurface;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> srfPoints;
  vtkm::cont::CellSetSingleType<> srfCells;
  vtkm::cont::CoordinateSystem slCoords("coordinates", res.Positions);
  streamSurface.Run(slCoords, res.PolyLines, srfPoints, srfCells);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outputCoords("coordinates", srfPoints);
  outData.AddCoordinateSystem(outputCoords);
  outData.SetCellSet(srfCells);

  return outData;
}

}
}
} // namespace vtkm::filter::flow
