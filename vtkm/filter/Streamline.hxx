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
inline VTKM_CONT Streamline::Streamline()
  : vtkm::filter::FilterDataSetWithField<Streamline>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void Streamline::SetSeeds(
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Streamline::DoExecute(
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

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  //todo: add check for rectilinear.
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  GridEvalType eval(coords, cells, field);
  RK4Type rk4(eval, static_cast<vtkm::worklet::particleadvection::ScalarType>(this->StepSize));

  vtkm::worklet::Streamline streamline;
  vtkm::worklet::StreamlineResult res;

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> seedArray;
  vtkm::cont::ArrayCopy(this->Seeds, seedArray);
  res = Worklet.Run(rk4, seedArray, this->NumberOfSteps);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outputCoords("coordinates", res.positions);
  outData.AddCellSet(res.polyLines);
  outData.AddCoordinateSystem(outputCoords);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Streamline::DoMapField(vtkm::cont::DataSet&,
                                             const vtkm::cont::ArrayHandle<T, StorageType>&,
                                             const vtkm::filter::FieldMetadata&,
                                             vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
