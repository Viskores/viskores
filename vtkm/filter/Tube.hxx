//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Tube_hxx
#define vtk_m_filter_Tube_hxx

#include <vtkm/filter/Tube.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Tube::Tube()
  : vtkm::filter::FilterDataSet<Tube>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet Tube::DoExecute(const vtkm::cont::DataSet& input,
                                                     vtkm::filter::PolicyBase<Policy> policy)
{
  this->Worklet.SetCapping(this->Capping);
  this->Worklet.SetNumberOfSides(this->NumberOfSides);
  this->Worklet.SetRadius(this->Radius);

  auto originalPoints = vtkm::filter::ApplyPolicyFieldOfType<vtkm::Vec3f>(
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()), policy, *this);
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  this->Worklet.Run(originalPoints, input.GetCellSet(), newPoints, newCells);

  vtkm::cont::DataSet outData;
  vtkm::cont::CoordinateSystem outCoords("coordinates", newPoints);
  outData.SetCellSet(newCells);
  outData.AddCoordinateSystem(outCoords);
  return outData;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool Tube::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                               const vtkm::cont::Field& field,
                                               vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(
      field, this->Worklet.GetOutputPointSourceIndex(), result);
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(
      field, this->Worklet.GetOutputCellSourceIndex(), result);
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
}
} // namespace vtkm::filter
#endif
