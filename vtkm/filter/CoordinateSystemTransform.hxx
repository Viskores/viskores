//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CoordianteSystemTransform_hxx
#define vtk_m_filter_CoordianteSystemTransform_hxx

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CylindricalCoordinateTransform::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& vtkmNotUsed(fieldMetadata),
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::ArrayHandle<T> outArray;
  vtkm::cont::DataSet outDataSet;

  this->Worklet.Run(field, outArray);

  // We first add the result coords to keep them at the first position
  // of the resulting dataset.
  outDataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", outArray));

  for (int i = 0; i < inDataSet.GetNumberOfCoordinateSystems(); i++)
  {
    outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(i));
  }

  outDataSet.SetCellSet(inDataSet.GetCellSet());

  return outDataSet;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet SphericalCoordinateTransform::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& vtkmNotUsed(fieldMetadata),
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::ArrayHandle<T> outArray;
  vtkm::cont::DataSet outDataSet;

  this->Worklet.Run(field, outArray);

  // We first add the result coords to keep them at the first position
  // of the resulting dataset.
  outDataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", outArray));

  for (int i = 0; i < inDataSet.GetNumberOfCoordinateSystems(); i++)
  {
    outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(i));
  }

  outDataSet.SetCellSet(inDataSet.GetCellSet());

  return outDataSet;
}
}
} // namespace vtkm::filter

#endif
