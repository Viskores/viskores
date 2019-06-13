//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

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
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  vtkm::cont::CellSetSingleType<> outCellSet(cells.GetName());
  vtkm::worklet::ExtractPoints worklet;

  outCellSet = worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                           coords.GetData(),
                           this->Function,
                           this->ExtractInside);

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
