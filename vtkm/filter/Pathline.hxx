//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Pathline_hxx
#define vtk_m_filter_Pathline_hxx

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Pathline::Pathline()
  : vtkm::filter::FilterDataSetWithField<Pathline>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void Pathline::SetSeeds(vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Pathline::DoExecute(
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
  const vtkm::cont::DynamicCellSet& cells2 = this->NextDataSet.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  const vtkm::cont::CoordinateSystem& coords2 =
    this->NextDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  auto field2 = vtkm::cont::Cast<vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>>(
    this->NextDataSet.GetField(this->GetActiveFieldName()).GetData());

  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  GridEvalType eval(
    coords, cells, field, this->PreviousTime, coords2, cells2, field2, this->NextTime);
  RK4Type rk4(eval, this->StepSize);

  vtkm::worklet::Streamline streamline;
  vtkm::worklet::StreamlineResult res;

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds, seedArray);
  res = Worklet.Run(rk4, seedArray, this->NumberOfSteps);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outputCoords("coordinates", res.Positions);
  outData.SetCellSet(res.PolyLines);
  outData.AddCoordinateSystem(outputCoords);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Pathline::DoMapField(vtkm::cont::DataSet&,
                                           const vtkm::cont::ArrayHandle<T, StorageType>&,
                                           const vtkm::filter::FieldMetadata&,
                                           vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
#endif
