//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractPoints_hxx
#define vtk_m_filter_ExtractPoints_hxx

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExtractPoints::ExtractPoints()
  : vtkm::filter::FilterDataSet<ExtractPoints>()
  , ExtractInside(true)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline vtkm::cont::DataSet ExtractPoints::DoExecute(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set and coordinates
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::ExtractPoints worklet;

  outCellSet = worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                           coords.GetData(),
                           this->Function,
                           this->ExtractInside);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ExtractPoints::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy);
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
  }

  // cell data does not apply
  return false;
}
}
}

#endif
