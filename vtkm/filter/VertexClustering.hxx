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

#include <vtkm/filter/VertexClustering.h>

#include <vtkm/filter/MapFieldPermutation.h>

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

  vtkm::cont::DataSet outDataSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicyCellSetUnstructured(input.GetCellSet(), policy, *this),
    input.GetCoordinateSystem(),
    bounds,
    this->GetNumberOfDivisions());

  return outDataSet;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool VertexClustering::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                           const vtkm::cont::Field& field,
                                                           vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetPointIdMap(), result);
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetCellIdMap(), result);
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
}
#endif
