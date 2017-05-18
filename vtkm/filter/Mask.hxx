//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

namespace
{
class AddPermutationCellSet
{
  vtkm::cont::DataSet* Output;
  vtkm::cont::ArrayHandle<vtkm::Id>* ValidIds;

public:
  AddPermutationCellSet(vtkm::cont::DataSet& data, vtkm::cont::ArrayHandle<vtkm::Id>& validIds)
    : Output(&data)
    , ValidIds(&validIds)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset) const
  {
    typedef vtkm::cont::CellSetPermutation<CellSetType> PermutationCellSetType;

    PermutationCellSetType permCellSet(*this->ValidIds, cellset, cellset.GetName());

    this->Output->AddCellSet(permCellSet);
  }
};
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Mask::Mask()
  : vtkm::filter::FilterDataSet<Mask>()
  , Stride(1)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::ResultDataSet Mask::DoExecute(
  const vtkm::cont::DataSet& input, const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

  vtkm::Id numberOfInputCells = cells.GetNumberOfCells();
  vtkm::Id numberOfSampledCells = numberOfInputCells / this->Stride;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> strideArray(0, this->Stride, numberOfSampledCells);

  DeviceAlgorithm::Copy(strideArray, this->ValidCellIds);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  AddPermutationCellSet addCellSet(output, this->ValidCellIds);
  vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicy(cells, policy), addCellSet);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool Mask::DoMapField(vtkm::filter::ResultDataSet& result,
                                       const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const vtkm::filter::FieldMetadata& fieldMeta,
                                       const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                       const DeviceAdapter&)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    result.GetDataSet().AddField(fieldMeta.AsField(input));
    return true;
  }

  if (fieldMeta.IsCellField())
  {
    //todo: We need to generate a new output policy that replaces
    //the original storage tag with a new storage tag where everything is
    //wrapped in ArrayHandlePermutation.
    typedef vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                               vtkm::cont::ArrayHandle<T, StorageType>>
      PermutationType;

    PermutationType permutation =
      vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input);

    result.GetDataSet().AddField(fieldMeta.AsField(permutation));
    return true;
  }

  // cell data does not apply
  return false;
}
}
}
