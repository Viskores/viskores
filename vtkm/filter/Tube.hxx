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

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorFilterExecution.h>

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
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Tube::DoMapField(vtkm::cont::DataSet& result,
                                       const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const vtkm::filter::FieldMetadata& fieldMeta,
                                       vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<T> fieldArray;

  if (fieldMeta.IsPointField())
    fieldArray = this->Worklet.ProcessPointField(input);
  else if (fieldMeta.IsCellField())
    fieldArray = this->Worklet.ProcessCellField(input);
  else
    return false;

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(fieldArray));
  return true;
}
}
} // namespace vtkm::filter
#endif
