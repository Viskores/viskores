//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_StreamSurface_hxx
#define vtk_m_filter_StreamSurface_hxx

#include <vtkm/filter/StreamSurface.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/Particles.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT StreamSurface::StreamSurface()
  : vtkm::filter::FilterDataSetWithField<StreamSurface>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet StreamSurface::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //Check for some basics.
  if (this->Seeds.GetNumberOfValues() == 0)
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");

  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using FieldType = vtkm::worklet::flow::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::flow::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::flow::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::flow::Stepper<RK4Type, GridEvalType>;

  //compute streamlines
  FieldType velocities(field, fieldMeta.GetAssociation());
  GridEvalType eval(coords, cells, velocities);
  Stepper rk4(eval, this->StepSize);

  vtkm::worklet::flow::Streamline streamline;

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds, seedArray);
  auto res = streamline.Run(rk4, seedArray, this->NumberOfSteps);

  //compute surface from streamlines
  vtkm::cont::ArrayHandle<vtkm::Vec3f> srfPoints;
  vtkm::cont::CellSetSingleType<> srfCells;
  vtkm::cont::CoordinateSystem slCoords("coordinates", res.Positions);
  this->Worklet.Run(slCoords, res.PolyLines, srfPoints, srfCells);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outputCoords("coordinates", srfPoints);
  outData.AddCoordinateSystem(outputCoords);
  outData.SetCellSet(srfCells);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool StreamSurface::MapFieldOntoOutput(vtkm::cont::DataSet&,
                                                        const vtkm::cont::Field&,
                                                        vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
