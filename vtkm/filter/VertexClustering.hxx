//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_VertexClustering_hxx
#define vtk_m_filter_VertexClustering_hxx

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT VertexClustering::VertexClustering()
  : vtkm::filter::FilterDataSet<VertexClustering>()
  , NumberOfDivisions(256, 256, 256)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet VertexClustering::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // todo this code needs to obey the policy for what storage types
  // the output should use
  //need to compute bounds first
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  vtkm::cont::DataSet outDataSet =
    this->Worklet.Run(vtkm::filter::ApplyPolicyCellSetUnstructured(input.GetCellSet(), policy),
                      input.GetCoordinateSystem(),
                      bounds,
                      this->GetNumberOfDivisions());

  return outDataSet;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool VertexClustering::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<T> fieldArray;

  if (fieldMeta.IsPointField())
  {
    fieldArray = this->Worklet.ProcessPointField(input);
  }
  else if (fieldMeta.IsCellField())
  {
    fieldArray = this->Worklet.ProcessCellField(input);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(fieldArray));

  return true;
}
}
}
#endif
