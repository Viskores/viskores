//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

namespace
{

// Needed to CompactPoints
template <typename BasePolicy>
struct CellSetSingleTypePolicy : public BasePolicy
{
  using AllCellSetList = vtkm::cont::CellSetListTagUnstructured;
};

template <typename DerivedPolicy>
inline vtkm::filter::PolicyBase<CellSetSingleTypePolicy<DerivedPolicy>> GetCellSetSingleTypePolicy(
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::filter::PolicyBase<CellSetSingleTypePolicy<DerivedPolicy>>();
}
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT MaskPoints::MaskPoints()
  : vtkm::filter::FilterDataSet<MaskPoints>()
  , Stride(1)
  , CompactPoints(true)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet MaskPoints::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  // run the worklet on the cell set and input field
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::MaskPoints worklet;

  outCellSet = worklet.Run(vtkm::filter::ApplyPolicy(cells, policy), this->Stride);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.DoExecute(output, GetCellSetSingleTypePolicy(policy));
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool MaskPoints::DoMapField(vtkm::cont::DataSet& result,
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
