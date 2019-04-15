//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExternalFaces::ExternalFaces()
  : vtkm::filter::FilterDataSet<ExternalFaces>()
  , CompactPoints(false)
  , Worklet()
{
  this->SetPassPolyData(true);
}

namespace
{

template <typename BasePolicy>
struct CellSetExplicitPolicy : public BasePolicy
{
  using AllCellSetList = vtkm::cont::CellSetListTagExplicitDefault;
};

template <typename DerivedPolicy>
inline vtkm::filter::PolicyBase<CellSetExplicitPolicy<DerivedPolicy>> GetCellSetExplicitPolicy(
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::filter::PolicyBase<CellSetExplicitPolicy<DerivedPolicy>>();
}

} // anonymous namespace

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ExternalFaces::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet(cells.GetName());

  if (cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    this->Worklet.Run(cells.Cast<vtkm::cont::CellSetStructured<3>>(),
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                      outCellSet);
  }
  else
  {
    this->Worklet.Run(vtkm::filter::ApplyPolicyUnstructured(cells, policy), outCellSet);
  }

  //3. Check the fields of the dataset to see what kinds of fields are present so
  //   we can free the cell mapping array if it won't be needed.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    if (f.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
    {
      hasCellFields = true;
    }
  }

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  //4. create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.DoExecute(output, GetCellSetExplicitPolicy(policy));
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ExternalFaces::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
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
  else if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> fieldArray;
    fieldArray = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }


  return false;
}
}
}
