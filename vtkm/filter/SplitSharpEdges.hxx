//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_SplitSharpEdges_hxx
#define vtk_m_filter_SplitSharpEdges_hxx
#include <vtkm/filter/SplitSharpEdges.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT SplitSharpEdges::SplitSharpEdges()
  : vtkm::filter::FilterDataSetWithField<SplitSharpEdges>()
  , FeatureAngle(30.0)
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet SplitSharpEdges::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& vtkmNotUsed(fieldMeta),
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // Get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newCoords;
  vtkm::cont::CellSetExplicit<> newCellset;

  this->Worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                    this->FeatureAngle,
                    field,
                    input.GetCoordinateSystem().GetDataAsMultiplexer(),
                    newCoords,
                    newCellset);

  vtkm::cont::DataSet output;
  output.SetCellSet(newCellset);
  output.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem(input.GetCoordinateSystem().GetName(), newCoords));
  return output;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT bool SplitSharpEdges::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                          const vtkm::cont::Field& field,
                                                          vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetNewPointsIdArray(), result);
  }
  else if (field.IsFieldCell() || field.IsFieldGlobal())
  {
    result.AddField(field); // pass through
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
