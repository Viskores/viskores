//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleAdvection_hxx
#define vtk_m_filter_ParticleAdvection_hxx

#include <vtkm/filter/ParticleAdvection.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ParticleAdvection::ParticleAdvection()
  : vtkm::filter::FilterDataSetWithField<ParticleAdvection>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void ParticleAdvection::SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ParticleAdvection::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //Check for some basics.
  if (this->Seeds.GetNumberOfValues() == 0)
  {
    throw vtkm::cont::ErrorFilterExecution("No seeds provided.");
  }

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  GridEvalType eval(coords, cells, field);
  RK4Type rk4(eval, this->StepSize);

  vtkm::worklet::ParticleAdvectionResult res;

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds, seedArray);
  res = this->Worklet.Run(rk4, seedArray, this->NumberOfSteps);

  vtkm::cont::DataSet outData;

  //Copy particles to coordinate array
  vtkm::cont::ArrayHandle<vtkm::Vec3f> outPos;
  vtkm::cont::ParticleArrayCopy(res.Particles, outPos);

  vtkm::cont::CoordinateSystem outCoords("coordinates", outPos);
  outData.AddCoordinateSystem(outCoords);

  //Create vertex cell set
  vtkm::Id numPoints = outPos.GetNumberOfValues();
  vtkm::cont::CellSetSingleType<> outCells;
  vtkm::cont::ArrayHandleIndex conn(numPoints);
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

  vtkm::cont::ArrayCopy(conn, connectivity);
  outCells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
  outData.SetCellSet(outCells);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool ParticleAdvection::MapFieldOntoOutput(vtkm::cont::DataSet&,
                                                            const vtkm::cont::Field&,
                                                            vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
